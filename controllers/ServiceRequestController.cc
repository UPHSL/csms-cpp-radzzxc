#include "ServiceRequestController.h"

#include <ctime>
#include <stdexcept>
#include <utility>
#include <vector>

namespace csms
{

namespace
{

using Callback = std::function<void(const drogon::HttpResponsePtr&)>;

// One form problem shown to the user: which field, and a readable message
using FieldError = std::pair<std::string, std::string>;

bool isBlank(const std::string& value)
{
    return value.find_first_not_of(" \t\n\r\f\v") == std::string::npos;
}

// Renders a CSP view (views/<name>.csp) with the given HTTP status
void respondWithView(Callback& callback, const std::string& viewName, const drogon::HttpViewData& data,
                     drogon::HttpStatusCode status = drogon::k200OK)
{
    auto resp = drogon::HttpResponse::newHttpViewResponse(viewName, data);
    resp->setStatusCode(status);
    callback(resp);
}

void respondNotFound(Callback& callback)
{
    drogon::HttpViewData data;
    data.insert("message", std::string("The requested Service Request could not be found."));
    respondWithView(callback, "ServiceRequestNotFound", data, drogon::k404NotFound);
}

// Success messages after a Post/Redirect/Get round trip (?msg=...)
std::string successMessageFor(const std::string& key)
{
    if (key == "submitted") return "Service Request submitted successfully.";
    if (key == "started")   return "Service Request is now In Progress.";
    if (key == "completed") return "Service Request completed successfully.";
    if (key == "cancelled") return "Service Request cancelled successfully.";
    return "";
}

// Wording for T09 validation field names. The rules themselves stay in ServiceRequestValidator.
std::string describeValidationError(const std::string& field)
{
    if (field == "residentId")    return "Resident ID must be a whole number greater than 0.";
    if (field == "serviceType")   return "Service type is required.";
    if (field == "description")   return "Description is required.";
    if (field == "dateRequested") return "Date requested must be a valid date (YYYY-MM-DD).";
    if (field == "status")        return "A new Service Request must start as Pending.";
    if (field == "id")            return "A new Service Request cannot have an ID yet.";
    return "Please check the " + field + " field.";
}

// Today's date (YYYY-MM-DD), used only to pre-fill the date field for convenience
std::string todayIso()
{
    std::time_t now = std::time(nullptr);
    char buffer[11] = {};
    if (const std::tm* local = std::localtime(&now))
    {
        std::strftime(buffer, sizeof(buffer), "%Y-%m-%d", local);
    }
    return buffer;
}

drogon::HttpViewData formData(const std::string& residentId, const std::string& serviceType,
                              const std::string& description, const std::string& dateRequested,
                              const std::vector<FieldError>& errors)
{
    drogon::HttpViewData data;
    data.insert("residentId", residentId);
    data.insert("serviceType", serviceType);
    data.insert("description", description);
    data.insert("dateRequested", dateRequested);
    data.insert("errors", errors);
    return data;
}

} // namespace

ServiceRequestController::ServiceRequestController()
    : ServiceRequestController(
          std::make_shared<ResidentRepository>("data/csms.db"),
          std::make_shared<ServiceRequestRepository>("data/csms.db"))
{
}

ServiceRequestController::ServiceRequestController(
    std::shared_ptr<ResidentRepository> residentRepository,
    std::shared_ptr<ServiceRequestRepository> serviceRequestRepository)
    : residentRepository_(std::move(residentRepository))
    , serviceRequestRepository_(std::move(serviceRequestRepository))
    , validator_()
    , queryService_(*serviceRequestRepository_)
    , submissionService_(validator_, *residentRepository_, *serviceRequestRepository_)
    , statusService_(*serviceRequestRepository_)
{
}

std::optional<int> ServiceRequestController::parseId(const std::string& value)
{
    if (value.empty() || value.size() > 9)
    {
        return std::nullopt;
    }
    int result = 0;
    for (char c : value)
    {
        if (c < '0' || c > '9')
        {
            return std::nullopt;
        }
        result = result * 10 + (c - '0');
    }
    if (result <= 0)
    {
        return std::nullopt;
    }
    return result;
}

void ServiceRequestController::index(const drogon::HttpRequestPtr& req, Callback&& callback)
{
    std::string searchTerm = req->getParameter("search");
    std::string statusFilter = req->getParameter("status");
    std::string errorMessage;

    std::optional<std::string> filter;
    if (!isBlank(statusFilter))
    {
        filter = statusFilter;
    }

    // Searching, filtering, blank-search handling and ordering all happen in T11
    std::vector<ServiceRequest> requests;
    try
    {
        requests = queryService_.searchServiceRequests(searchTerm, filter);
    }
    catch (const std::invalid_argument&)
    {
        // T11 rejected an unknown status (e.g. a hand-typed URL); show all statuses instead of failing
        errorMessage = "\"" + statusFilter + "\" is not a valid status filter. Showing all statuses.";
        statusFilter.clear();
        requests = queryService_.searchServiceRequests(searchTerm, std::nullopt);
    }

    drogon::HttpViewData data;
    data.insert("requests", requests);
    data.insert("search", searchTerm);
    data.insert("status", statusFilter);
    data.insert("successMessage", successMessageFor(req->getParameter("msg")));
    data.insert("errorMessage", errorMessage);
    respondWithView(callback, "ServiceRequestIndex", data);
}

void ServiceRequestController::showSubmitForm(const drogon::HttpRequestPtr&, Callback&& callback)
{
    respondWithView(callback, "ServiceRequestForm", formData("", "", "", todayIso(), {}));
}

void ServiceRequestController::submit(const drogon::HttpRequestPtr& req, Callback&& callback)
{
    const std::string residentIdText = req->getParameter("residentId");
    const std::string serviceType = req->getParameter("serviceType");
    const std::string description = req->getParameter("description");
    const std::string dateRequested = req->getParameter("dateRequested");

    // A Resident ID that isn't a whole number becomes 0, which T09 reports as invalid.
    // ID and status are never read from the form: new requests start as Pending (T08/T09).
    ServiceRequest request(parseId(residentIdText).value_or(0), serviceType, description, dateRequested);

    const ServiceRequestSubmissionResult result = submissionService_.submitServiceRequest(request);

    if (result.success)
    {
        // Post/Redirect/Get
        callback(drogon::HttpResponse::newRedirectionResponse("/service-requests?msg=submitted", drogon::k303SeeOther));
        return;
    }

    std::vector<FieldError> errors;
    for (const auto& field : result.validationErrors)
    {
        errors.emplace_back(field, describeValidationError(field));
    }
    if (result.residentNotFound)
    {
        errors.emplace_back("residentId", "Resident not found. Check the Resident ID on the Residents page.");
    }
    if (result.residentInactive)
    {
        errors.emplace_back("residentId", "Inactive Residents cannot submit new Service Requests.");
    }

    respondWithView(callback, "ServiceRequestForm",
                    formData(residentIdText, serviceType, description, dateRequested, errors));
}

void ServiceRequestController::show(const drogon::HttpRequestPtr& req, Callback&& callback, const std::string& idStr)
{
    const std::optional<int> id = parseId(idStr);
    if (!id)
    {
        respondNotFound(callback);
        return;
    }

    const std::optional<ServiceRequest> request = queryService_.viewServiceRequest(*id);
    if (!request)
    {
        respondNotFound(callback);
        return;
    }

    drogon::HttpViewData data;
    data.insert("request", request);
    data.insert("successMessage", successMessageFor(req->getParameter("msg")));
    data.insert("errorMessage", std::string());
    respondWithView(callback, "ServiceRequestDetail", data);
}

void ServiceRequestController::startProcessing(const drogon::HttpRequestPtr& req, Callback&& callback, const std::string& idStr)
{
    changeStatus(req, std::move(callback), idStr, "In Progress", "started");
}

void ServiceRequestController::complete(const drogon::HttpRequestPtr& req, Callback&& callback, const std::string& idStr)
{
    changeStatus(req, std::move(callback), idStr, "Completed", "completed");
}

void ServiceRequestController::cancel(const drogon::HttpRequestPtr& req, Callback&& callback, const std::string& idStr)
{
    changeStatus(req, std::move(callback), idStr, "Cancelled", "cancelled");
}

void ServiceRequestController::changeStatus(
    const drogon::HttpRequestPtr& req,
    Callback&& callback,
    const std::string& idStr,
    const std::string& targetStatus,
    const std::string& successKey)
{
    const std::optional<int> id = parseId(idStr);
    if (!id)
    {
        respondNotFound(callback);
        return;
    }

    // T10 decides whether this transition is allowed
    const ServiceRequestStatusResult result = statusService_.updateStatus(*id, targetStatus);

    if (result.notFound)
    {
        respondNotFound(callback);
        return;
    }

    if (result.success)
    {
        // Post/Redirect/Get: back to the list or the details page, wherever the action came from
        const std::string location = (req->getParameter("returnTo") == "list")
            ? "/service-requests?msg=" + successKey
            : "/service-requests/" + std::to_string(*id) + "?msg=" + successKey;
        callback(drogon::HttpResponse::newRedirectionResponse(location, drogon::k303SeeOther));
        return;
    }

    // T10 rejected the transition (e.g. a hand-made request to complete a Pending request).
    // Nothing was changed; show the request again with a controlled error.
    const std::optional<ServiceRequest> current = queryService_.viewServiceRequest(*id);
    if (!current)
    {
        respondNotFound(callback);
        return;
    }

    drogon::HttpViewData data;
    data.insert("request", current);
    data.insert("successMessage", std::string());
    data.insert("errorMessage", "That action isn't available for a request that is " + current->getStatus() +
                                    ". No changes were made.");
    respondWithView(callback, "ServiceRequestDetail", data, drogon::k409Conflict);
}

} // namespace csms
