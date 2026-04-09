/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

// Local Includes
#include "mountservice.h"
#include "syscall.h"

// External Includes
#include <nap/core.h>
#include <nap/logger.h>

RTTI_BEGIN_CLASS(nap::MountServiceConfiguration)
    RTTI_PROPERTY("Enabled",    &nap::MountServiceConfiguration::mEnabled,      nap::rtti::EPropertyMetaData::Default, "If the mounter is activated and initialized on startup" ) ;
    RTTI_PROPERTY("ReadOnly",   &nap::MountServiceConfiguration::mReadOnly,     nap::rtti::EPropertyMetaData::Default, "Mount disk read-only, otherwise read-write")
    RTTI_PROPERTY("MountPoint", &nap::MountServiceConfiguration::mMountPoint,   nap::rtti::EPropertyMetaData::Default, "Mount location (root)")
	RTTI_PROPERTY("Exclusions", &nap::MountServiceConfiguration::mExclusions,   nap::rtti::EPropertyMetaData::Default, "Disk UUID or labels to exclude");
	RTTI_PROPERTY("Inclusions", &nap::MountServiceConfiguration::mInclusions,   nap::rtti::EPropertyMetaData::Default, "Disk UUID or labels to include; empty = include all");
    RTTI_PROPERTY("Frequency",  &nap::MountServiceConfiguration::mFrequency,    nap::rtti::EPropertyMetaData::Default, "How often to check for disk changes in seconds")
RTTI_END_CLASS

RTTI_BEGIN_CLASS_NO_DEFAULT_CONSTRUCTOR(nap::MountService)
	RTTI_CONSTRUCTOR(nap::ServiceConfiguration*)
RTTI_END_CLASS

namespace nap
{
	//////////////////////////////////////////
	// Static
	//////////////////////////////////////////

    using DiskID = MountService::DiskID;

    static bool unmount(const std::string& point)
    {
        // USER MUST HAVE 'NOPASSWD' permission for 'Umount' cmd!
        // ie: cd /etc/sudoers.d; sudo nano pocketdvs; user ALL=(ALL) NOPASSWD:/bin/mount, /bin/umount, /usr/sbin/blkid
        auto umount_cmd = utility::stringFormat(R"(sudo -n umount "%s")", point.c_str());
        return system(umount_cmd.c_str()) == 0;
    }


    static bool mounted(const std::string& point)
    {
        if (utility::dirExists(point))
        {
            auto check_cmd = utility::stringFormat(R"(mountpoint -q "%s")", point.c_str());
            return system(check_cmd.c_str()) == 0;
        }
        return false;
    }


    static bool mount(const std::string& uuid, const std::string& point, bool readOnly)
    {
        // Unmount point if a disk is mounted there
        if (mounted(point) && !unmount(point))
            Logger::error("Failed to unmount '%s'", point.c_str());

        // USER MUST HAVE 'NOPASSWD' permission for 'mount' cmd!
        // ie: cd /etc/sudoers.d; sudo nano pocketdvs; user ALL=(ALL) NOPASSWD:/bin/mount, /bin/umount, /usr/sbin/blkid
        auto mount_cmd = readOnly ?
            utility::stringFormat(R"(sudo -n mount -r UUID="%s" "%s")", uuid.c_str(), point.c_str()):
            utility::stringFormat(R"(sudo -n mount UUID="%s" "%s")", uuid.c_str(), point.c_str());

        return system(mount_cmd.c_str()) == 0;
    }


    static bool removeDir(const std::string& path)
    {
        if (utility::dirExists(path))
        {
            const auto del_cmd = utility::stringFormat(R"(sudo -n rmdir "%s")", path.c_str());
            return system(del_cmd.c_str()) == 0;
        }
        return true;
    }


    static std::string getDiskProperty(const std::string& entry, const char* property)
    {
        // Check if it contains property
        auto fc = entry.find(property);
        if (fc != std::string::npos)
        {
            // Get property value
            fc += strlen(property);
            assert(fc < entry.size() && entry[fc] == '\"');
            auto lc = entry.find_first_of('\"', ++fc);
            if (lc != std::string::npos)
            {
                return entry.substr(fc, lc - fc);
            }
            Logger::error("Property '%s' parsing error, entry ''", property, entry.c_str());
        }
        return "";
    }


    static std::vector<DiskID> listDisks()
    {
        // USER MUST HAVE 'NOPASSWD' permission for 'mount' cmd!
        // ie: cd /etc/sudoers.d; sudo nano pocketdvs; user ALL=(ALL) NOPASSWD:/bin/mount, /bin/umount, /usr/sbin/blkid
        static constexpr const char* rawDeviceUUID = "PTUUID=";
        static constexpr const char* virtualDeviceUUID= "UUID=";
        static constexpr const char* labelID = "LABEL=";

        // Find all block devices (rc of 2 = no devices)
        std::string response;
        static constexpr const char* blkidcmd = "sudo blkid /dev/sd*";
        auto rc = utility::syscall(blkidcmd, response);
        if (rc != utility::rc::success && rc != 2)
        {
            Logger::error("Unable to list disk directory, ec %d", rc);
            return {};
        }

        // Get disks that are eligable for mounting
        Logger::debug(response.empty() ?
            utility::stringFormat("CMD '%s' response: Empty", blkidcmd ) :
            utility::stringFormat("CMD '%s' response: \n%s", blkidcmd, response.c_str())
            );

        // Bail if there are no entries
        if (response.empty()) { return {}; }

        auto entries = utility::splitString(response, '\n');
        std::vector<DiskID> disks; disks.reserve(entries.size());
        for (const auto& entry: entries)
        {
            // Skip raw block devices
            auto buid = getDiskProperty(entry, rawDeviceUUID);
            if (!buid.empty())
            {
                Logger::debug("Skipping raw block device '%s'", entry.c_str());
                continue;
            }

            // Get UUID (required!)
            auto uuid = getDiskProperty(entry, virtualDeviceUUID);
            if (uuid.empty())
            {
                Logger::warn("Missing '%s' for block device '%s'",
                    virtualDeviceUUID, entry.c_str());
                continue;
            }

            // Get Label (optional!)
            auto label = getDiskProperty(entry, labelID);
            disks.emplace_back(std::move(uuid), std::move(label));
        }
        return disks;
    }


    //////////////////////////////////////////
    // Service
    //////////////////////////////////////////

	bool MountService::init(nap::utility::ErrorState& errorState)
	{
		mConfig = getConfiguration<MountServiceConfiguration>();
        mMountPoint = mConfig->mMountPoint;

        // Create inclusion & exclusion maps
		mInclusionMap.reserve(mConfig->mInclusions.size());
		for (const auto& i : mConfig->mInclusions)
			mInclusionMap.emplace(i);

		mExclusionMap.reserve(mConfig->mExclusions.size());
		for (const auto& e : mConfig->mExclusions)
			mExclusionMap.emplace(e);

        // Bail if we're not enabled
        if (!mConfig->mEnabled)
            return true;

        // Ensure disk by uuid directory exists
        if (!errorState.check(utility::dirExists(diskRoot),
            "Unable to install drive watcher, directory '%s' doesn't exist!", diskRoot))
            return false;

        // Mount drives
        diff();

        // Install directory watcher
        Logger::info("Installing drive watcher on '%s'", diskRoot);
        mWatcher = std::make_unique<DirectoryWatcher>(diskRoot);
        mTimer.reset();
		return true;
	}


	void MountService::update(double deltaTime)
	{
        // File watcher unavailable or window not met
        assert(mConfig != nullptr);
        if (mWatcher == nullptr || mTimer.getElapsedTime() < mConfig->mFrequency)
            return;

        // Diff mounted disks when changes are detected
        mModifiedPaths.clear();
        if (mWatcher->update(mModifiedPaths)) {
            diff();
        }

        // Reset timer
        mTimer.reset();
	}
	

	void MountService::shutdown()
	{
        for (const auto& entry : mMountMap)
        {
            if(!unmount(entry.second)) {
                Logger::error("Unable to unmount '%s'", entry.second.c_str());
            }
        }
	}


    std::vector<std::string> MountService::getDrives() const
    {
        std::vector<std::string> disks; disks.reserve(mMountMap.size());
        for (const auto& mp : mMountMap)
            disks.emplace_back(mp.second);
        return disks;
    }


    void MountService::diff()
    {
        // Check if the disks by label exists
        Logger::info("Diffing disks at '%s'", diskRoot);

        // Get disks
        auto disks = listDisks();

        // Remove the ones that aren't available anymore
        for (auto mp = mMountMap.begin(); mp != mMountMap.end();)
        {
            // Check if there's an entry in the mounting map
            auto fit = std::find_if(disks.begin(), disks.end(), [&mp](const auto& disk) {
                return mp->first == disk.first;
            });

            // If there is, we don't purge it
            if (fit != disks.end()) {
                ++mp; continue;
            }

            // Delete entry & notify listeners
            Logger::info("Removed drive '%s' at '%s'", mp->first.c_str(), mp->second.c_str());
            std::string target = mp->second;
            mp = mMountMap.erase(mp);
            driveRemoved(mp->second); configChanged();

            // unmount, delete and remove it
            if (!unmount(target))
                Logger::error("Unable to unmount '%s'", mp->second.c_str());
        }

        // Mount new drives
        for (auto& disk : disks)
        {
            // Continue if directory is excluded
            if (!isIncluded(disk) || isExcluded(disk))
            {
                Logger::debug("Excluding '%s:%s' as mount option",
                    disk.first.c_str(), disk.second.empty() ? "?" : disk.second.c_str());
                continue;
            }

            // Disk ID & Label (Reverts to Disk when not found)
            const auto& di = disk.first;
            const auto& dl = disk.second.empty() ? diskLabel : disk.second;

            // Check if this disk is already mounted
            if (mMountMap.find(di) != mMountMap.end())
                continue;

            // Create mount point -> must be user writable
            auto mp = mountpoint(dl);
            if (!utility::ensureDirExists(mp))
            {
                Logger::error("Unable to create directory '%s'", mp.c_str());
                continue;
            }

            // Mount disk at given directory
            if (!mount(di, mp, getReadOnly()))
            {
                Logger::error("Unable to mount disk '%s' at '%s'", di.c_str(), mp.c_str());
                continue;
            }

            // Add as valid map entry
            Logger::info("Mounted drive '%s:%s' at '%s'", di.c_str(), dl.c_str(), mp.c_str());

            auto [fst, snd] = mMountMap.emplace(di, mp); assert(snd);
            driveAdded(fst->second); configChanged();
        }
    }


    std::string MountService::mountpoint(const std::string& label, int rec)
    {
        // Try to find a non-existing mount location -> append index when looped back
        auto loc = utility::stringFormat("%s/%s", mMountPoint.c_str(), label.c_str());
        loc = rec > 0 ? utility::stringFormat("%s_%d", loc.c_str(), rec+1) : loc;

        // Check if it doesn't exist yet
        const auto it = std::find_if(mMountMap.begin(), mMountMap.end(), [&loc](const auto& mp) {
            return mp.second == loc;
        });

        return it == mMountMap.end() ? loc : mountpoint(label, ++rec);
    }


    bool MountService::isIncluded(const DiskID& id)
    {
        if (!mInclusionMap.empty())
        {
            auto in_uuids = mInclusionMap.find(id.first)  != mInclusionMap.end();
            auto in_label = mInclusionMap.find(id.second) != mInclusionMap.end();
            return in_uuids || in_label;
        }
        return true;
    }


    bool MountService::isExcluded(const DiskID& id)
    {
        auto in_uuids = mExclusionMap.find(id.first)  != mExclusionMap.end();
        auto in_label = mExclusionMap.find(id.second) != mExclusionMap.end();
        return in_uuids || in_label;
    }
}
