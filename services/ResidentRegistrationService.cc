#include "ResidentRegistrationService.h"

namespace csms
{

// Constructor: initialize validator and repository references
ResidentRegistrationService::ResidentRegistrationService(
    const ResidentValidator& validator,
    ResidentRepository& repository
)
    : validator_(validator),
      repository_(repository)
{
}

ResidentRegistrationResult ResidentRegistrationService::registerResident(
    const Resident& resident
)
{
    // Step 1: Validate resident before attempting any database persistence
    const std::vector<std::string> errors = validator_.validate(resident);

    // Step 2: Reject invalid resident information early
    // Prevent invalid data from ever reaching the repository / database
    if (!errors.empty())
    {
        return ResidentRegistrationResult{
            false,        // success = false
            std::nullopt, // no persisted resident
            errors        // return identified validation errors
        };
    }

    // Step 3: Persist valid resident via the existing repository abstraction
    // SQLite assigns the auto-increment identifier during this step
    Resident persistedResident = repository_.save(resident);

    // Step 4: Return successful result containing the persisted entity with its ID
    return ResidentRegistrationResult{
        true,              // success = true
        persistedResident, // resident containing generated database ID
        {}                 // no errors
    };
}

} // namespace csms
