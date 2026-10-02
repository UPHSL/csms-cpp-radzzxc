#pragma once

#include <optional>
#include <string>
#include <vector>

#include "models/Resident.h"

namespace csms
{

// Represents the outcome of an attempted resident update.
// Distinguishes among three primary results:
// 1. Success (success = true, notFound = false, resident = updated data)
// 2. Validation failure (success = false, notFound = false, errors = list of invalid fields)
// 3. Resident not found (success = false, notFound = true, errors = empty)
struct ResidentUpdateResult
{
    // True if update succeeded and was persisted
    bool success{false};

    // True if the targeted resident ID was not found in persistence
    bool notFound{false};

    // Holds the updated persisted Resident if successful; std::nullopt otherwise
    std::optional<Resident> resident{std::nullopt};

    // Contains failing field names (e.g., "firstName", "contactNumber") if validation fails
    std::vector<std::string> errors;
};

} // namespace csms
