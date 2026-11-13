#pragma once

#include "models/Resident.h"
#include "models/ResidentValidator.h"
#include "repositories/ResidentRepository.h"
#include "ResidentRegistrationResult.h"

namespace csms
{

// Application service that orchestrates the resident registration workflow.
// Coordinates domain validation and database persistence without duplicating
// their respective responsibilities.
class ResidentRegistrationService
{
public:
    // Dependency injection constructor: receives references to existing
    // validator and repository rather than instantiating them internally.
    ResidentRegistrationService(
        const ResidentValidator& validator,
        ResidentRepository& repository
    );

    // Orchestrates registration:
    // 1. Validates the resident using ResidentValidator
    // 2. If invalid, halts and returns failure with validation errors (database is untouched)
    // 3. If valid, persists using ResidentRepository and returns success with assigned ID
    ResidentRegistrationResult registerResident(
        const Resident& resident
    );

private:
    const ResidentValidator& validator_;
    ResidentRepository& repository_;
};

} // namespace csms
