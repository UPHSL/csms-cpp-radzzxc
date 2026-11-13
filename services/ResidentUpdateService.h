#pragma once

#include <string>
#include "models/Resident.h"
#include "models/ResidentValidator.h"
#include "repositories/ResidentRepository.h"
#include "ResidentUpdateResult.h"

namespace csms
{

// Application service that orchestrates updating an existing resident's personal/contact information.
// Coordinates domain validation and database persistence without altering immutable fields (ID, status).
class ResidentUpdateService
{
public:
    // Dependency injection constructor: receives references to existing validator and repository
    ResidentUpdateService(
        const ResidentValidator& validator,
        ResidentRepository& repository
    );

    // Primary update operation: accepts target ID and a Resident object containing proposed editable fields
    ResidentUpdateResult updateResident(
        int id,
        const Resident& proposed
    );

    // Convenience overload: accepts individual editable fields
    ResidentUpdateResult updateResident(
        int id,
        const std::string& firstName,
        const std::string& lastName,
        const std::string& address,
        const std::string& contactNumber,
        const std::string& email
    );

private:
    const ResidentValidator& validator_;
    ResidentRepository& repository_;
};

} // namespace csms
