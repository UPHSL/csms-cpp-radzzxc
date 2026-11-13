#pragma once

#include <vector>
#include "models/ServiceRequest.h"

namespace csms
{

// T14: Read-only snapshot of the current CSMS state, built by DashboardService.
// Every number comes from a persistence-level COUNT query; nothing here is stored separately.
struct DashboardSummary
{
    // Resident summary (Total = Active + Inactive; Inactive residents are still persisted)
    int totalResidents{0};
    int activeResidents{0};
    int inactiveResidents{0};

    // Service Request summary (Total = Pending + In Progress + Completed + Cancelled)
    int totalServiceRequests{0};
    int pendingServiceRequests{0};
    int inProgressServiceRequests{0};
    int completedServiceRequests{0};
    int cancelledServiceRequests{0};

    // Derived metric, not a stored status: Open = Pending + In Progress
    int openServiceRequests{0};

    // Up to five newest requests, ordered dateRequested DESC, then id DESC (T11 ordering)
    std::vector<ServiceRequest> recentServiceRequests{};
};

} // namespace csms
