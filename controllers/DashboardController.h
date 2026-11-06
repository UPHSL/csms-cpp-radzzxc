#pragma once

#include <drogon/HttpController.h>
#include <memory>
#include "repositories/ResidentRepository.h"
#include "repositories/ServiceRequestRepository.h"
#include "services/DashboardService.h"

namespace csms
{

// T14 presentation layer for the CSMS Dashboard.
// Receives GET /dashboard, asks DashboardService for the summary, and renders views/Dashboard.csp.
// It owns no SQL, no counting logic and no business rules, and it never changes persisted data.
class DashboardController : public drogon::HttpController<DashboardController>
{
public:
    METHOD_LIST_BEGIN
    // GET /dashboard -> read-only operational summary
    ADD_METHOD_TO(DashboardController::index, "/dashboard", drogon::Get);
    METHOD_LIST_END

    // Default constructor uses the application database
    DashboardController();

    // Dependency-injection constructor for tests and isolation
    DashboardController(
        std::shared_ptr<ResidentRepository> residentRepository,
        std::shared_ptr<ServiceRequestRepository> serviceRequestRepository
    );

    void index(
        const drogon::HttpRequestPtr& req,
        std::function<void(const drogon::HttpResponsePtr&)>&& callback
    );

private:
    std::shared_ptr<ResidentRepository> residentRepository_;
    std::shared_ptr<ServiceRequestRepository> serviceRequestRepository_;
    DashboardService dashboardService_;
};

} // namespace csms
