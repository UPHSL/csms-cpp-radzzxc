#pragma once

#include <drogon/HttpController.h>
#include <memory>
#include <optional>
#include <string>
#include "models/ServiceRequestValidator.h"
#include "repositories/ResidentRepository.h"
#include "repositories/ServiceRequestRepository.h"
#include "services/ServiceRequestQueryService.h"
#include "services/ServiceRequestStatusService.h"
#include "services/ServiceRequestSubmissionService.h"

namespace csms
{

// T13 presentation layer for Service Requests.
// Reads browser input, calls the existing T09 (submission), T10 (status workflow)
// and T11 (query) services, and renders the CSP views in views/ServiceRequest*.csp.
// It owns no SQL and no business rules.
class ServiceRequestController : public drogon::HttpController<ServiceRequestController>
{
public:
    METHOD_LIST_BEGIN
    // GET /service-requests -> list, text search (?search=) and status filter (?status=)
    ADD_METHOD_TO(ServiceRequestController::index, "/service-requests", drogon::Get);

    // GET /service-requests/new -> submission form
    ADD_METHOD_TO(ServiceRequestController::showSubmitForm, "/service-requests/new", drogon::Get);

    // POST /service-requests -> submit a new request (T09)
    ADD_METHOD_TO(ServiceRequestController::submit, "/service-requests", drogon::Post);

    // GET /service-requests/{id} -> details (T11 view)
    ADD_METHOD_TO(ServiceRequestController::show, "/service-requests/{1}", drogon::Get);

    // POST workflow actions (T10). Each maps to one target status; T10 decides if it is allowed.
    ADD_METHOD_TO(ServiceRequestController::startProcessing, "/service-requests/{1}/start", drogon::Post);
    ADD_METHOD_TO(ServiceRequestController::complete, "/service-requests/{1}/complete", drogon::Post);
    ADD_METHOD_TO(ServiceRequestController::cancel, "/service-requests/{1}/cancel", drogon::Post);
    METHOD_LIST_END

    // Default constructor uses the application database
    ServiceRequestController();

    // Dependency-injection constructor for tests and isolation
    ServiceRequestController(
        std::shared_ptr<ResidentRepository> residentRepository,
        std::shared_ptr<ServiceRequestRepository> serviceRequestRepository
    );

    void index(
        const drogon::HttpRequestPtr& req,
        std::function<void(const drogon::HttpResponsePtr&)>&& callback
    );

    void showSubmitForm(
        const drogon::HttpRequestPtr& req,
        std::function<void(const drogon::HttpResponsePtr&)>&& callback
    );

    void submit(
        const drogon::HttpRequestPtr& req,
        std::function<void(const drogon::HttpResponsePtr&)>&& callback
    );

    void show(
        const drogon::HttpRequestPtr& req,
        std::function<void(const drogon::HttpResponsePtr&)>&& callback,
        const std::string& idStr
    );

    void startProcessing(
        const drogon::HttpRequestPtr& req,
        std::function<void(const drogon::HttpResponsePtr&)>&& callback,
        const std::string& idStr
    );

    void complete(
        const drogon::HttpRequestPtr& req,
        std::function<void(const drogon::HttpResponsePtr&)>&& callback,
        const std::string& idStr
    );

    void cancel(
        const drogon::HttpRequestPtr& req,
        std::function<void(const drogon::HttpResponsePtr&)>&& callback,
        const std::string& idStr
    );

    // Parses a whole, positive number ("12" -> 12). Anything else ("", "12abc", "-3") -> nullopt.
    static std::optional<int> parseId(const std::string& value);

private:
    // Shared handler for the three workflow actions
    void changeStatus(
        const drogon::HttpRequestPtr& req,
        std::function<void(const drogon::HttpResponsePtr&)>&& callback,
        const std::string& idStr,
        const std::string& targetStatus,
        const std::string& successKey
    );

    std::shared_ptr<ResidentRepository> residentRepository_;
    std::shared_ptr<ServiceRequestRepository> serviceRequestRepository_;
    ServiceRequestValidator validator_;
    ServiceRequestQueryService queryService_;
    ServiceRequestSubmissionService submissionService_;
    ServiceRequestStatusService statusService_;
};

} // namespace csms
