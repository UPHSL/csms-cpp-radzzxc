#pragma once

#include "models/Resident.h"
#include "repositories/ResidentRepository.h"
#include "ResidentDeactivationResult.h"

namespace csms
{

// Application service that orchestrates the controlled soft deactivation of an existing resident.
// Transitions status from Active to Inactive while strictly preserving ID and all personal/contact details.
class ResidentDeactivationService
{
public:
    // Dependency injection constructor: receives reference to ResidentRepository
    explicit ResidentDeactivationService(ResidentRepository& repository);

    // Primary deactivation operation: accepts resident ID and coordinates status transition
    ResidentDeactivationResult deactivateResident(int residentId);

private:
    ResidentRepository& repository_;
};

} // namespace csms
