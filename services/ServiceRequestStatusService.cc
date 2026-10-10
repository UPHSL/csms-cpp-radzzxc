#include "ServiceRequestStatusService.h"

namespace csms
{

ServiceRequestStatusService::ServiceRequestStatusService(ServiceRequestRepository& repository)
    : repository_(repository)
{
}

bool ServiceRequestStatusService::isSupportedStatus(const std::string& status)
{
    return status == "Pending" ||
           status == "In Progress" ||
           status == "Completed" ||
           status == "Cancelled";
}

bool ServiceRequestStatusService::isAllowedTransition(const std::string& currentStatus, const std::string& targetStatus)
{
    if (currentStatus == targetStatus)
    {
        return false;
    }

    if (currentStatus == "Pending")
    {
        return targetStatus == "In Progress" || targetStatus == "Cancelled";
    }

    if (currentStatus == "In Progress")
    {
        return targetStatus == "Completed" || targetStatus == "Cancelled";
    }

    // "Completed" and "Cancelled" are terminal states; no further transitions allowed
    return false;
}

ServiceRequestStatusResult ServiceRequestStatusService::updateStatus(int requestId, const std::string& newStatus)
{
    // Step 1: Retrieve the existing Service Request from persistence
    auto existingOpt = repository_.findById(requestId);
    if (!existingOpt.has_value())
    {
        return ServiceRequestStatusResult{
            .success = false,
            .notFound = true,
            .unsupportedStatus = false,
            .invalidTransition = false,
            .message = "Service request not found.",
            .serviceRequest = std::nullopt
        };
    }

    // Step 2: Determine whether the requested status is supported
    if (!isSupportedStatus(newStatus))
    {
        return ServiceRequestStatusResult{
            .success = false,
            .notFound = false,
            .unsupportedStatus = true,
            .invalidTransition = false,
            .message = "Unsupported target status: " + newStatus,
            .serviceRequest = std::nullopt
        };
    }

    // Step 3: Determine whether the requested transition is allowed
    const std::string& currentStatus = existingOpt->getStatus();
    if (!isAllowedTransition(currentStatus, newStatus))
    {
        return ServiceRequestStatusResult{
            .success = false,
            .notFound = false,
            .unsupportedStatus = false,
            .invalidTransition = true,
            .message = "Invalid status transition from '" + currentStatus + "' to '" + newStatus + "'.",
            .serviceRequest = std::nullopt
        };
    }

    // Step 4: Persist the valid status change in SQLite
    bool updated = repository_.updateStatus(requestId, newStatus);
    if (!updated)
    {
        return ServiceRequestStatusResult{
            .success = false,
            .notFound = true,
            .unsupportedStatus = false,
            .invalidTransition = false,
            .message = "Service request no longer exists; status was not updated.",
            .serviceRequest = std::nullopt
        };
    }

    // Step 5: Retrieve and return the updated persisted Service Request
    auto updatedRequest = repository_.findById(requestId);

    return ServiceRequestStatusResult{
        .success = true,
        .notFound = false,
        .unsupportedStatus = false,
        .invalidTransition = false,
        .message = "Status updated successfully.",
        .serviceRequest = updatedRequest
    };
}

} // namespace csms
