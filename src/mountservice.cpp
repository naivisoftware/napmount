// Local Includes
#include "mountservice.h"

// External Includes
#include <nap/core.h>
#include <nap/resourcemanager.h>
#include <nap/logger.h>
#include <iostream>

RTTI_BEGIN_CLASS_NO_DEFAULT_CONSTRUCTOR(nap::mountService)
	RTTI_CONSTRUCTOR(nap::ServiceConfiguration*)
RTTI_END_CLASS

namespace nap
{
	bool mountService::init(nap::utility::ErrorState& errorState)
	{
		//Logger::info("Initializing mountService");
		return true;
	}


	void mountService::update(double deltaTime)
	{
	}
	

	void mountService::getDependentServices(std::vector<rtti::TypeInfo>& dependencies)
	{
	}
	

	void mountService::shutdown()
	{
	}
}
