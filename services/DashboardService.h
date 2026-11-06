#pragma once

#include "repositories/ResidentRepository.h"
#include "repositories/ServiceRequestRepository.h"
#include "DashboardSummary.h"

namespace csms
{

// T14 application query service that composes the CSMS Dashboard.
// It asks the repositories for aggregate counts and the recent requests, derives the
// Open Service Request count, and returns a DashboardSummary.
// Read-only: it never validates, creates, updates, deactivates or changes the status of anything,
// renders no HTML, and does not manage database connections.
class DashboardService
{
public:
    // Number of rows shown in the "Recent Service Requests" section
    static constexpr int kRecentServiceRequestLimit = 5;

    // Dependency injection constructor: receives references to the existing repositories
    DashboardService(ResidentRepository& residentRepository, ServiceRequestRepository& serviceRequestRepository);

    // Builds the current dashboard summary from persisted data
    DashboardSummary getSummary();

private:
    ResidentRepository& residentRepository_;
    ServiceRequestRepository& serviceRequestRepository_;
};

} // namespace csms
