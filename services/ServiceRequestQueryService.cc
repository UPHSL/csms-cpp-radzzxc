#include "ServiceRequestQueryService.h"
#include <stdexcept>

namespace csms
{

ServiceRequestQueryService::ServiceRequestQueryService(ServiceRequestRepository& repository)
    : repository_(repository)
{
}

std::vector<ServiceRequest> ServiceRequestQueryService::listServiceRequests()
{
    return repository_.findAll();
}

std::optional<ServiceRequest> ServiceRequestQueryService::viewServiceRequest(int id)
{
    return repository_.findById(id);
}

std::vector<ServiceRequest> ServiceRequestQueryService::searchServiceRequests(
    const std::string& searchTerm,
    const std::optional<std::string>& statusFilter)
{
    // Step 1: Normalize search term by removing leading and trailing whitespace
    const std::string normalizedSearchTerm = trim(searchTerm);

    // Step 2: Validate and normalize status filter if provided
    std::optional<std::string> normalizedStatus;
    if (statusFilter.has_value())
    {
        const std::string trimmedStatus = trim(*statusFilter);
        if (!trimmedStatus.empty())
        {
            if (!isSupportedStatus(trimmedStatus))
            {
                throw std::invalid_argument("Unsupported status filter: " + trimmedStatus);
            }
            normalizedStatus = trimmedStatus;
        }
    }

    // Step 3: Blank text search with no status filter delegates to listServiceRequests()
    if (normalizedSearchTerm.empty() && !normalizedStatus.has_value())
    {
        return listServiceRequests();
    }

    // Step 4: Delegate filtered/searched query to repository persistence layer
    return repository_.search(normalizedSearchTerm, normalizedStatus);
}

bool ServiceRequestQueryService::isSupportedStatus(const std::string& status)
{
    return status == "Pending" ||
           status == "In Progress" ||
           status == "Completed" ||
           status == "Cancelled";
}

std::string ServiceRequestQueryService::trim(const std::string& value)
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