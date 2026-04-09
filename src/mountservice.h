#pragma once

// External Includes
#include <nap/service.h>
#include <unordered_set>
#include <nap/signalslot.h>
#include <nap/directorywatcher.h>
#include <nap/timer.h>
#include <cassert>

namespace nap
{
	// Forward declares
	class MountService;

	/**
	 * Mount service configuration structure
	 */
	class NAPAPI MountServiceConfiguration : public ServiceConfiguration
	{
		RTTI_ENABLE(ServiceConfiguration)
	public:
		MountServiceConfiguration() = default;
		bool mEnabled = true;								///< Property: 'Enabled' If the mounter is activated and initialized on startup
		std::vector<std::string> mExclusions;				///< Property: 'Exclusions' Disk UUID or labels to exclude
		std::vector<std::string> mInclusions;				///< Property: 'Inclusions' Disk UUID or labels to include, include all when left empty
		std::string mMountPoint = "/tmp/napmount";			///< Property: 'MountRoot' mount point root directory
		float mFrequency = 1.0f;							///< Property: 'Frequency' How often to check for disk changes in seconds
		bool mReadOnly = true;								///< Property: 'ReadOnly' Mount read-only, otherwise read-write

		rtti::TypeInfo getServiceType() const override		{ return RTTI_OF(MountService); }
	};


	/**
	 * Watches /dev/disk/by-uuid and mounts disks from that directory at the given mount-point.
	 * Linux Only!
	 *
	 * This object requires sudo NOPASSWD privileges for the following commands:
	 * user ALL=(ALL) NOPASSWD:/bin/mount, /bin/umount, /usr/sbin/blkid
	 */
	class NAPAPI MountService : public Service
	{
		RTTI_ENABLE(Service)
	public:
		static constexpr const char* diskRoot = "/dev/disk/by-uuid";       //< Disk root directory
		static constexpr const char* diskLabel = "DISK";                   //< Default disk label

		// Default Constructor
		MountService(ServiceConfiguration* configuration) : Service(configuration)	{ }

		
		/**
		 * Initialize the mounter and mount drives
		 * @param errorState contains the error message on failure
		 * @return if the video service was initialized correctly
		 */
		bool init(nap::utility::ErrorState& errorState) override;
		
		/**
		 * Updates mount map at fixed interval.
		 * Listen to the various signals (driveAdded, etc.) to receive mount updates.
		 * @param deltaTime: the time in seconds between calls
		 */
		void update(double deltaTime) override;
		
		/**
		 * Unmounts all currently mounted drives
		 */
		void shutdown() override;

		/**
		 * @return Paths to all mounted drives
		 */
		std::vector<std::string> getDrives() const;

		/**
		 * @return Mounted drives, UUID & mount point
		 */
		using MountMap = std::unordered_map<std::string, std::string>;
		const MountMap& getMap() const { return mMountMap;}

		/**
		 * @return if the mounter is enabled
		 */
		bool getEnabled() const { assert(mConfig != nullptr); return mConfig->mEnabled; }

		/**
		 * @return if drives are mounted read-only, otherwise read-write
		 */
		bool getReadOnly() const { assert(mConfig != nullptr); return mConfig->mReadOnly; }

		// Called when a drive is mounted
		Signal<const std::string&> driveAdded;

		// Called when a drive is removed
		Signal<const std::string&> driveRemoved;

		// Called when the drive config has changed
		Signal<> configChanged;

		// DISK UUID (required) & Label (optional)
		using DiskID = std::pair<const std::string, const std::string>;

	private:
		std::unordered_set<std::string> mExclusionMap; // Disk label or uuid to exclude
		std::unordered_set<std::string> mInclusionMap; // Disk label or uuid to include, empty = all
		MountServiceConfiguration* mConfig = nullptr;

		MountMap mMountMap;
		std::string mMountPoint;;
		std::unique_ptr<DirectoryWatcher> mWatcher = nullptr;
		HighResolutionTimer mTimer;
		std::vector<std::string> mModifiedPaths;

		void diff();
		std::string mountpoint(const std::string& label, int rec = 0);
		bool isIncluded(const DiskID& id);
		bool isExcluded(const DiskID& id);
	};
}
