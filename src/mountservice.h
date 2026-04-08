#pragma once

// External Includes
#include <nap/service.h>
#include <unordered_set>
#include <nap/signalslot.h>
#include <nap/directorywatcher.h>
#include <nap/timer.h>

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
		std::vector<std::string> mExclusions;				///< Property: 'Exclusions' Disk UUID or labels to exclude
		std::vector<std::string> mInclusions;				///< Property: 'Inclusions' Disk UUID or labels to include, include all when left empty
		std::string mMountPoint = "/tmp/napmount";			///< Property: 'MountRoot' mount point root directory

		rtti::TypeInfo getServiceType() const override		{ return RTTI_OF(MountService); }
	};


	class NAPAPI MountService : public Service
	{
		RTTI_ENABLE(Service)
	public:
		static constexpr const char* diskRoot = "/dev/disk/by-uuid";       //< Disk root directory
		static constexpr const char* diskLabel = "DISK";                   //< Default disk label

		// Default Constructor
		MountService(ServiceConfiguration* configuration) : Service(configuration)	{ }

		
		/**
		 * Initialize the mounter
		 * @param errorState contains the error message on failure
		 * @return if the video service was initialized correctly
		 */
		bool init(nap::utility::ErrorState& errorState) override;
		
		/**
		 * Invoked by core in the app loop. Update order depends on service dependency
		 * This call is invoked after the resource manager has loaded any file changes but before
		 * the app update call. If service B depends on A, A:s:update() is called before B::update()
		 * @param deltaTime: the time in seconds between calls
		*/
		void update(double deltaTime) override;
		
		/**
		 * Invoked when exiting the main loop, after app shutdown is called
		 * Use this function to close service specific handles, drivers or devices
		 * When service B depends on A, Service B is shutdown before A
		 */
		void shutdown() override;

		// Called when a disk is added or removed
		Signal<const std::string&> driveAdded;
		Signal<const std::string&> driveRemoved;
		Signal<> configChanged;

		// DISK UUID (required) & Label (optional)
		using DiskID = std::pair<const std::string, const std::string>;

	private:
		// DISK UUID & -> Point
		using MountMap = std::unordered_map<std::string, std::string>;

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
