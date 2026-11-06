#include "DashboardController.h"

#include <exception>
#include <optional>
#include <string>
#include <utility>

namespace csms
{

DashboardController::DashboardController()
    : DashboardController(
          std::make_shared<ResidentRepository>("data/csms.db"),
          std::make_shared<ServiceRequestRepository>("data/csms.db"))
{
}

DashboardController::DashboardController(
    std::shared_ptr<ResidentRepository> residentRepository,
    std::shared_ptr<ServiceRequestRepository> serviceRequestRepository)
    : residentRepository_(std::move(residentRepository))
    , serviceRequestRepository_(std::move(serviceRequestRepository))
    , dashboardService_(*residentRepository_, *serviceRequestRepository_)
{
}

void DashboardController::index(const drogon::HttpRequestPtr&,
                                std::function<void(const drogon::HttpResponsePtr&)>&& callback)
{
    std::optional<DashboardSummary> summary;
    std::string errorMessage;
    drogon::HttpStatusCode status = drogon::k200OK;

    try
    {
        summary = dashboardService_.getSummary();
    }
    catch (const std::exception&)
    {
        // Never show database details to the user; the page explains the problem in plain words
        errorMessage = "The dashboard could not be loaded right now. Please refresh the page or try again later.";
        status = drogon::k500InternalServerError;
    }

    drogon::HttpViewData data;
    data.insert("summary", summary);
    data.insert("errorMessage", errorMessage);

    auto resp = drogon::HttpResponse::newHttpViewResponse("Dashboard", data);
    resp->setStatusCode(status);
    callback(resp);
}

} // namespace csms
