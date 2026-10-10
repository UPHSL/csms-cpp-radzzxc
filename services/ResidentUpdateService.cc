#include "ResidentUpdateService.h"

namespace csms
{

ResidentUpdateService::ResidentUpdateService(
    const ResidentValidator& validator,
    ResidentRepository& repository
)
    : validator_(validator),
      repository_(repository)
{
}

ResidentUpdateResult ResidentUpdateService::updateResident(
    int id,
    const Resident& proposed
)
{
    // Step 1: Look up existing resident by ID in persistence
    std::optional<Resident> existing = repository_.findById(id);

    // Step 2: Handle nonexistent resident safely
    // Prevent accidental insert/upsert; return not-found result immediately
    if (!existing.has_value())
    {
        return ResidentUpdateResult{
            false,        // success = false
            true,         // notFound = true
            std::nullopt, // no resident
            {}            // no validation errors
        };
    }

    // Step 3: Build update candidate preserving original ID and original Status
    // Only firstName, lastName, address, contactNumber, and email are updated
    Resident candidate(
        proposed.getFirstName(),
        proposed.getLastName(),
        proposed.getAddress(),
        proposed.getContactNumber(),
        proposed.getEmail(),
        existing->getStatus(), // preserve existing status (Active or Inactive)
        id                     // preserve original database ID
    );

    // Step 4: Validate candidate using existing T02 rules BEFORE modifying persistence
    const std::vector<std::string> errors = validator_.validate(candidate);

    // Step 5: Reject invalid updates early; database remains untouched
    if (!errors.empty())
    {
        return ResidentUpdateResult{
            false,        // success = false
            false,        // notFound = false
            std::nullopt, // no updated resident
            errors        // details of validation failures
        };
    }

    // Step 6: Persist valid changes to the existing record
    bool updated = repository_.update(candidate);
    if (!updated)
    {
        return ResidentUpdateResult{
            false,        // success = false
            true,         // notFound = true
            std::nullopt, // no updated resident
            {}            // no validation errors
        };
    }

    // Step 7: Return successful result containing the updated entity
    return ResidentUpdateResult{
        true,         // success = true
        false,        // notFound = false
        candidate,    // updated resident
        {}            // no errors
    };
}

ResidentUpdateResult ResidentUpdateService::updateResident(
    int id,
    const std::string& firstName,
    const std::string& lastName,
    const std::string& address,
    const std::string& contactNumber,
    const std::string& email
)
{
    Resident proposed(firstName, lastName, address, contactNumber, email);
    return updateResident(id, proposed);
}

} // namespace csms
