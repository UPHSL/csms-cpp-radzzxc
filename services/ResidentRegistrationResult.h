#pragma once

#include <optional>
#include <string>
#include <vector>

#include "models/Resident.h"

namespace csms
{

// Represents the outcome of an attempted resident registration.
// Instead of returning only a boolean (true/false), this struct encapsulates
// whether the operation succeeded, the persisted resident (if successful),
// and any validation error messages (if validation failed).
struct ResidentRegistrationResult
{
    // Indicates if registration succeeded
    bool success{false};

    // Holds the persisted Resident with its database-generated ID upon success;
    // remains std::nullopt if registration failed.
    std::optional<Resident> resident{std::nullopt};

    // Contains field names or messages that failed validation (e.g., "firstName");
    // remains empty if registration succeeded.
    std::vector<std::string> errors;
};

} // namespace csms
