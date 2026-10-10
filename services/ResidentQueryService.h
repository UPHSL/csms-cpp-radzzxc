#pragma once

#include <string>
#include <vector>

#include "models/Resident.h"
#include "repositories/ResidentRepository.h"

namespace csms
{

// Application query service coordinating resident listing and searching operations.
// Encapsulates query logic (e.g., whitespace trimming, routing blank searches to listing)
// and coordinates with ResidentRepository for persistent data retrieval.
class ResidentQueryService
{
public:
    // Dependency injection constructor: receives reference to existing ResidentRepository
    explicit ResidentQueryService(
        ResidentRepository& repository
    );

    // Lists all persisted residents ordered by lastName ASC, firstName ASC, id ASC
    std::vector<Resident> listResidents();

    // Searches residents by first name or last name (case-insensitive partial match).
    // If the trimmed search term is empty, redirects to listResidents().
    std::vector<Resident> searchResidents(
        const std::string& searchTerm
    );

private:
    ResidentRepository& repository_;

    // Helper: Trims leading and trailing whitespace characters from input string
    static std::string trim(
        const std::string& value
    );
};

} // namespace csms
