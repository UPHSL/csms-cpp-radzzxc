#include "DashboardService.h"

namespace csms
{

DashboardService::DashboardService(ResidentRepository& residentRepository,
                                   ServiceRequestRepository& serviceRequestRepository)
    : residentRepository_(residentRepository)
    , serviceRequestRepository_(serviceRequestRepository)
{
}

DashboardSummary DashboardService::getSummary()
{
    DashboardSummary summary;

    // Resident aggregates (COUNT queries in ResidentRepository)
    summary.totalResidents = residentRepository_.countAll();
    summary.activeResidents = residentRepository_.countByStatus("Active");
    summary.inactiveResidents = residentRepository_.countByStatus("Inactive");

    // Service Request aggregates (COUNT queries in ServiceRequestRepository)
    summary.totalServiceRequests = serviceRequestRepository_.countAll();
    summary.pendingServiceRequests = serviceRequestRepository_.countByStatus("Pending");
    summary.inProgressServiceRequests = serviceRequestRepository_.countByStatus("In Progress");
    summary.completedServiceRequests = serviceRequestRepository_.countByStatus("Completed");
    summary.cancelledServiceRequests = serviceRequestRepository_.countByStatus("Cancelled");

    // Derived metric: Completed and Cancelled are terminal, so only these two are still open
    summary.openServiceRequests = summary.pendingServiceRequests + summary.inProgressServiceRequests;

    // Newest requests, ordered and limited by the repository (T11 ordering + SQL LIMIT)
    summary.recentServiceRequests = serviceRequestRepository_.findRecent(kRecentServiceRequestLimit);

    return summary;
}

} // namespace csms
