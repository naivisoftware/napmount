// Local Includes
#include "mountservice.h"
#include "syscall.h"

// External Includes
#include <nap/core.h>
#include <nap/logger.h>

RTTI_BEGIN_CLASS(nap::MountServiceConfiguration)
	RTTI_PROPERTY("Exclusions", &nap::MountServiceConfiguration::mExclusions, nap::rtti::EPropertyMetaData::Default);
	RTTI_PROPERTY("Inclusions", &nap::MountServiceConfiguration::mInclusions, nap::rtti::EPropertyMetaData::Default);
RTTI_END_CLASS

RTTI_BEGIN_CLASS_NO_DEFAULT_CONSTRUCTOR(nap::MountService)
	RTTI_CONSTRUCTOR(nap::ServiceConfiguration*)
RTTI_END_CLASS

namespace nap
{
    // UUID (required) & Label (optional)
    using DiskID = std::pair<const std::string, const std::string>;

	//////////////////////////////////////////
	// Static
	//////////////////////////////////////////

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


    static bool mount(const std::string& uuid, const std::string& point)
    {
        // Unmount point if a disk is mounted there
        if (mounted(point) && !unmount(point))
            Logger::error("Failed to unmount '%s'", point.c_str());

        // USER MUST HAVE 'NOPASSWD' permission for 'mount' cmd!
        // ie: cd /etc/sudoers.d; sudo nano pocketdvs; user ALL=(ALL) NOPASSWD:/bin/mount, /bin/umount, /usr/sbin/blkid
        auto mount_cmd = utility::stringFormat(R"(sudo -n mount -r UUID="%s" "%s")", uuid.c_str(), point.c_str());
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


	bool MountService::init(nap::utility::ErrorState& errorState)
	{
		mConfig = getConfiguration<MountServiceConfiguration>();

		mInclusionMap.reserve(mConfig->mInclusions.size());
		for (const auto& i : mConfig->mInclusions)
			mInclusionMap.emplace(i);

		mExclusionMap.reserve(mConfig->mExclusions.size());
		for (const auto& e : mConfig->mExclusions)
			mExclusionMap.emplace(e);

		//Logger::info("Initializing mountService");
		return true;
	}


	void MountService::update(double deltaTime)
	{
	}
	

	void MountService::getDependentServices(std::vector<rtti::TypeInfo>& dependencies)
	{
	}
	

	void MountService::shutdown()
	{
	}
}
