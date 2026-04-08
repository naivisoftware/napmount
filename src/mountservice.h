#pragma once

// External Includes
#include <nap/service.h>
#include <unordered_set>

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

		rtti::TypeInfo getServiceType() const override		{ return RTTI_OF(MountService); }
	};


	class NAPAPI MountService : public Service
	{
		RTTI_ENABLE(Service)
	public:
		// Default Constructor
		MountService(ServiceConfiguration* configuration) : Service(configuration)	{ }

		/**
		 * Use this call to register service dependencies
		 * A service that depends on another service is initialized after all it's associated dependencies
		 * This will ensure correct order of initialization, update calls and shutdown of all services
		 * @param dependencies rtti information of the services this service depends on
		 */
		void getDependentServices(std::vector<rtti::TypeInfo>& dependencies) override;
		
		/**
		 * Initializes the service
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

	private:
		std::unordered_set<std::string> mExclusionMap; // Disk label or uuid to exclude
		std::unordered_set<std::string> mInclusionMap; // Disk label or uuid to include, empty = all
		MountServiceConfiguration* mConfig = nullptr;
	};
}
