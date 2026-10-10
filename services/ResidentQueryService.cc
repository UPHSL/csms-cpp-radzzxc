#include "ResidentQueryService.h"

namespace csms
{

ResidentQueryService::ResidentQueryService(
    ResidentRepository& repository
)
    : repository_(repository)
{
}

std::vector<Resident> ResidentQueryService::listResidents()
{
    // Delegate directly to the repository persistence layer
    return repository_.findAll();
}

std::vector<Resident> ResidentQueryService::searchResidents(
    const std::string& searchTerm
)
{
    // Step 1: Normalize search input by removing leading and trailing whitespace
    const std::string normalizedSearchTerm = trim(searchTerm);

    // Step 2: Blank or whitespace-only searches act as a request to list all residents
    if (normalizedSearchTerm.empty())
    {
        return listResidents();
    }

    // Step 3: Non-blank search delegates to the repository for database-level query filtering
    return repository_.searchByName(normalizedSearchTerm);
}

std::string ResidentQueryService::trim(
    const std::string& value
)
{
    const std::string whitespace = " \t\n\r\f\v";

    const std::size_t first = value.find_first_not_of(whitespace);
    if (first == std::string::npos)
    {
        return "";
    }

    const std::size_t last = value.find_last_not_of(whitespace);
    return value.substr(first, last - first + 1);
}

} // namespace csms
