#pragma once

#include <string>
#include <vector>
#include <optional>

#include "models/ServiceRequest.h"
#include "repositories/ServiceRequestRepository.h"

namespace csms
{

// Application query service coordinating service request listing, viewing, searching, and filtering operations.
// Encapsulates query logic (e.g., whitespace trimming, blank-search routing, status validation)
// and coordinates with ServiceRequestRepository for persistent data retrieval.
class ServiceRequestQueryService
{
public:
    // Dependency injection constructor: receives reference to existing ServiceRequestRepository
    explicit ServiceRequestQueryService(ServiceRequestRepository& repository);

    // Lists all persisted service requests ordered by dateRequested DESC, id DESC
    std::vector<ServiceRequest> listServiceRequests();

    // Retrieves one persisted service request by its ID safely
    std::optional<ServiceRequest> viewServiceRequest(int id);

    // Searches service requests by serviceType or description (case-insensitive partial match),
    // optionally filtering by status.
    // If the trimmed search term is empty and no status filter is provided, redirects to listServiceRequests().
    std::vector<ServiceRequest> searchServiceRequests(
        const std::string& searchTerm = "",
        const std::optional<std::string>& statusFilter = std::nullopt
    );

    // Validates whether the given status string is one of the four supported lifecycle states
    static bool isSupportedStatus(const std::string& status);

private:
    ServiceRequestRepository& repository_;

    // Helper: Trims leading and trailing whitespace characters from input string
    static std::string trim(const std::string& value);
};

} // namespace csms