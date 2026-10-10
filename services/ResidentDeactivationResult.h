#pragma once

#include <optional>
#include "models/Resident.h"

namespace csms
{

// Represents the outcome of an attempted resident deactivation.
// Distinguishes among three primary results required by T07:
// 1. Success (success = true, alreadyInactive = false, notFound = false, resident = deactivated record)
// 2. Already Inactive (success = true, alreadyInactive = true, notFound = false, resident = existing record)
// 3. Resident not found (success = false, alreadyInactive = false, notFound = true, resident = std::nullopt)
struct ResidentDeactivationResult
{
    // True if the resident is Inactive in persistence (newly deactivated or already inactive)
    bool success{false};

    // True if the resident was already Inactive prior to this operation (safe repeated deactivation)
    bool alreadyInactive{false};

    // True if the targeted resident ID was not found in persistence
    bool notFound{false};

    // Holds the persisted Resident after the operation; std::nullopt if not found
    std::optional<Resident> resident{std::nullopt};
};

} // namespace csms
