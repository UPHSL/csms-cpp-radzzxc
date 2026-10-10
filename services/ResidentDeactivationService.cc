#include "ResidentDeactivationService.h"

namespace csms
{

ResidentDeactivationService::ResidentDeactivationService(ResidentRepository& repository)
    : repository_(repository)
{
}

ResidentDeactivationResult ResidentDeactivationService::deactivateResident(int residentId)
{
    // 1. Verify existence of the resident in persistence
    auto existing = repository_.findById(residentId);
    if (!existing.has_value())
    {
        return ResidentDeactivationResult{
            .success = false,
            .alreadyInactive = false,
            .notFound = true,
            .resident = std::nullopt
        };
    }

    // 2. Check if resident is already Inactive (idempotent / safe repeated deactivation)
    if (existing->getStatus() == "Inactive")
    {
        return ResidentDeactivationResult{
            .success = true,
            .alreadyInactive = true,
            .notFound = false,
            .resident = existing
        };
    }

    // 3. Deactivate the active resident in persistence
    bool updated = repository_.deactivateById(residentId);
    if (!updated)
    {
        return ResidentDeactivationResult{
            .success = false,
            .alreadyInactive = false,
            .notFound = false,
            .resident = std::nullopt
        };
    }

    // 4. Retrieve and return the final persisted Resident state
    auto updatedResident = repository_.findById(residentId);
    return ResidentDeactivationResult{
        .success = true,
        .alreadyInactive = false,
        .notFound = false,
        .resident = updatedResident
    };
}

} // namespace csms
