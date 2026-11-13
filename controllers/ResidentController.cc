#include "ResidentController.h"

#include <utility>
#include <vector>

namespace csms
{

namespace
{

using Callback = std::function<void(const drogon::HttpResponsePtr&)>;

// One form problem shown to the user: which field, and a readable message
using FieldError = std::pair<std::string, std::string>;

// Renders a CSP view (views/<name>.csp) with the given HTTP status
void respondWithView(Callback& callback, const std::string& viewName, const drogon::HttpViewData& data,
                     drogon::HttpStatusCode status = drogon::k200OK)
{
    auto resp = drogon::HttpResponse::newHttpViewResponse(viewName, data);
    resp->setStatusCode(status);
    callback(resp);
}

void respondNotFound(Callback& callback, const std::string& message)
{
    drogon::HttpViewData data;
    data.insert("message", message);
    respondWithView(callback, "ResidentNotFound", data, drogon::k404NotFound);
}

// Wording for T02 validation field names. The rules themselves stay in ResidentValidator.
std::string describeError(const std::string& field)
{
    if (field == "firstName")     return "First name is required.";
    if (field == "lastName")      return "Last name is required.";
    if (field == "address")       return "Address is required.";
    if (field == "contactNumber") return "Contact number must follow the format 09XXXXXXXXX.";
    if (field == "email")         return "Email address is invalid (example: name@domain.com).";
    if (field == "status")        return "Resident status is invalid.";
    return field;
}

// View data for the five editable fields plus any validation errors
drogon::HttpViewData formData(const std::string& firstName, const std::string& lastName, const std::string& address,
                              const std::string& contactNumber, const std::string& email,
                              const std::vector<std::string>& errorFields)
{
    std::vector<FieldError> errors;
    for (const auto& field : errorFields)
    {
        errors.emplace_back(field, describeError(field));
    }

    drogon::HttpViewData data;
    data.insert("firstName", firstName);
    data.insert("lastName", lastName);
    data.insert("address", address);
    data.insert("contactNumber", contactNumber);
    data.insert("email", email);
    data.insert("errors", errors);
    return data;
}

// Adds the read-only details shown at the top of the Edit page
void addEditDetails(drogon::HttpViewData& data, const Resident& resident)
{
    data.insert("residentId", resident.getId() ? std::to_string(*resident.getId()) : std::string("-"));
    data.insert("displayName", resident.getFirstName() + " " + resident.getLastName());
    data.insert("status", resident.getStatus());
}

// Parses a whole, positive number ("12" -> 12). Anything else ("", "12abc", "-3") -> nullopt.
std::optional<int> parseId(const std::string& value)
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

} // namespace

ResidentController::ResidentController()
    : ResidentController(std::make_shared<ResidentRepository>("data/csms.db"))
{
}

ResidentController::ResidentController(std::shared_ptr<ResidentRepository> repository)
    : repository_(repository ? std::move(repository) : std::make_shared<ResidentRepository>("data/csms.db"))
    , validator_(std::make_shared<ResidentValidator>())
    , queryService_(std::make_unique<ResidentQueryService>(*repository_))
    , registrationService_(std::make_unique<ResidentRegistrationService>(*validator_, *repository_))
    , updateService_(std::make_unique<ResidentUpdateService>(*validator_, *repository_))
    , deactivationService_(std::make_unique<ResidentDeactivationService>(*repository_))
{
}

void ResidentController::index(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback)
{
    std::string searchTerm = req->getParameter("search");
    std::vector<Resident> residents;

    if (!searchTerm.empty())
    {
        residents = queryService_->searchResidents(searchTerm);
    }
    else
    {
        residents = queryService_->listResidents();
    }

    std::string successMessage;
    std::string msgKey = req->getParameter("msg");
    if (msgKey == "registered")
    {
        successMessage = "Resident registered successfully.";
    }
    else if (msgKey == "updated")
    {
        successMessage = "Resident information updated successfully.";
    }
    else if (msgKey == "deactivated")
    {
        successMessage = "Resident deactivated successfully.";
    }
    else if (msgKey == "already-inactive")
    {
        successMessage = "This resident is already inactive. No changes were made.";
    }

    drogon::HttpViewData data;
    data.insert("residents", residents);
    data.insert("search", searchTerm);
    data.insert("successMessage", successMessage);
    respondWithView(callback, "ResidentIndex", data);
}

void ResidentController::showRegisterForm(
    const drogon::HttpRequestPtr&,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback)
{
    respondWithView(callback, "ResidentRegister", formData("", "", "", "", "", {}));
}

void ResidentController::registerResident(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback)
{
    std::string firstName = req->getParameter("firstName");
    std::string lastName = req->getParameter("lastName");
    std::string address = req->getParameter("address");
    std::string contactNumber = req->getParameter("contactNumber");
    std::string email = req->getParameter("email");

    Resident resident(firstName, lastName, address, contactNumber, email);
    auto result = registrationService_->registerResident(resident);

    if (result.success)
    {
        // Post/Redirect/Get pattern
        auto resp = drogon::HttpResponse::newRedirectionResponse("/residents?msg=registered", drogon::k303SeeOther);
        callback(resp);
        return;
    }

    // Preserve entered non-sensitive values and display validation errors
    respondWithView(callback, "ResidentRegister",
                    formData(firstName, lastName, address, contactNumber, email, result.errors));
}

void ResidentController::showEditForm(
    const drogon::HttpRequestPtr&,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
    const std::string& idStr)
{
    auto idOpt = parseId(idStr);
    if (!idOpt.has_value())
    {
        respondNotFound(callback, "Invalid resident identifier.");
        return;
    }
    int residentId = idOpt.value();

    auto residentOpt = repository_->findById(residentId);
    if (!residentOpt.has_value())
    {
        respondNotFound(callback, "The requested resident record could not be found.");
        return;
    }

    // Prefill the form with the persisted values
    const Resident& resident = residentOpt.value();
    auto data = formData(resident.getFirstName(), resident.getLastName(), resident.getAddress(),
                         resident.getContactNumber(), resident.getEmail(), {});
    addEditDetails(data, resident);
    respondWithView(callback, "ResidentEdit", data);
}

void ResidentController::updateResident(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
    const std::string& idStr)
{
    auto idOpt = parseId(idStr);
    if (!idOpt.has_value())
    {
        respondNotFound(callback, "Invalid resident identifier.");
        return;
    }
    int residentId = idOpt.value();

    auto residentOpt = repository_->findById(residentId);
    if (!residentOpt.has_value())
    {
        respondNotFound(callback, "The requested resident record could not be found.");
        return;
    }

    std::string firstName = req->getParameter("firstName");
    std::string lastName = req->getParameter("lastName");
    std::string address = req->getParameter("address");
    std::string contactNumber = req->getParameter("contactNumber");
    std::string email = req->getParameter("email");

    auto result = updateService_->updateResident(
        residentId, firstName, lastName, address, contactNumber, email
    );

    if (result.success)
    {
        // Post/Redirect/Get pattern
        auto resp = drogon::HttpResponse::newRedirectionResponse("/residents?msg=updated", drogon::k303SeeOther);
        callback(resp);
        return;
    }

    // Show what the user typed, with validation errors; the heading/ID/status stay the persisted ones
    auto data = formData(firstName, lastName, address, contactNumber, email, result.errors);
    addEditDetails(data, residentOpt.value());
    respondWithView(callback, "ResidentEdit", data);
}

void ResidentController::deactivateResident(
    const drogon::HttpRequestPtr&,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
    const std::string& idStr)
{
    auto idOpt = parseId(idStr);
    if (!idOpt.has_value())
    {
        respondNotFound(callback, "Invalid resident identifier.");
        return;
    }
    int residentId = idOpt.value();

    auto result = deactivationService_->deactivateResident(residentId);

    if (result.notFound)
    {
        respondNotFound(callback, "The requested resident record could not be found.");
        return;
    }

    // Post/Redirect/Get pattern. A repeated request is safe (T07) but gets its own message.
    const char* target = result.alreadyInactive ? "/residents?msg=already-inactive" : "/residents?msg=deactivated";
    auto resp = drogon::HttpResponse::newRedirectionResponse(target, drogon::k303SeeOther);
    callback(resp);
}

} // namespace csms
