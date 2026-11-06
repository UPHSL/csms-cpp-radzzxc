#include "ResidentController.h"
#include "views/ResidentView.h"

namespace csms
{

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

    std::string html = ResidentView::renderIndex(residents, searchTerm, successMessage);

    auto resp = drogon::HttpResponse::newHttpResponse();
    resp->setStatusCode(drogon::k200OK);
    resp->setContentTypeCode(drogon::CT_TEXT_HTML);
    resp->setBody(std::move(html));
    callback(resp);
}

void ResidentController::showRegisterForm(
    const drogon::HttpRequestPtr&,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback)
{
    std::string html = ResidentView::renderRegisterForm();

    auto resp = drogon::HttpResponse::newHttpResponse();
    resp->setStatusCode(drogon::k200OK);
    resp->setContentTypeCode(drogon::CT_TEXT_HTML);
    resp->setBody(std::move(html));
    callback(resp);
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
    ResidentView::FormState state;
    state.firstName = firstName;
    state.lastName = lastName;
    state.address = address;
    state.contactNumber = contactNumber;
    state.email = email;
    state.errors = result.errors;

    std::string html = ResidentView::renderRegisterForm(state);

    auto resp = drogon::HttpResponse::newHttpResponse();
    resp->setStatusCode(drogon::k200OK);
    resp->setContentTypeCode(drogon::CT_TEXT_HTML);
    resp->setBody(std::move(html));
    callback(resp);
}

void ResidentController::showEditForm(
    const drogon::HttpRequestPtr&,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
    const std::string& idStr)
{
    int residentId = -1;
    try
    {
        residentId = std::stoi(idStr);
    }
    catch (...)
    {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k404NotFound);
        resp->setContentTypeCode(drogon::CT_TEXT_HTML);
        resp->setBody(ResidentView::renderNotFound("Invalid resident identifier."));
        callback(resp);
        return;
    }

    auto residentOpt = repository_->findById(residentId);
    if (!residentOpt.has_value())
    {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k404NotFound);
        resp->setContentTypeCode(drogon::CT_TEXT_HTML);
        resp->setBody(ResidentView::renderNotFound("The requested resident record could not be found."));
        callback(resp);
        return;
    }

    std::string html = ResidentView::renderEditForm(residentOpt.value());

    auto resp = drogon::HttpResponse::newHttpResponse();
    resp->setStatusCode(drogon::k200OK);
    resp->setContentTypeCode(drogon::CT_TEXT_HTML);
    resp->setBody(std::move(html));
    callback(resp);
}

void ResidentController::updateResident(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
    const std::string& idStr)
{
    int residentId = -1;
    try
    {
        residentId = std::stoi(idStr);
    }
    catch (...)
    {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k404NotFound);
        resp->setContentTypeCode(drogon::CT_TEXT_HTML);
        resp->setBody(ResidentView::renderNotFound("Invalid resident identifier."));
        callback(resp);
        return;
    }

    auto residentOpt = repository_->findById(residentId);
    if (!residentOpt.has_value())
    {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k404NotFound);
        resp->setContentTypeCode(drogon::CT_TEXT_HTML);
        resp->setBody(ResidentView::renderNotFound("The requested resident record could not be found."));
        callback(resp);
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

    // Preserve updated values and display validation errors
    ResidentView::FormState state;
    state.firstName = firstName;
    state.lastName = lastName;
    state.address = address;
    state.contactNumber = contactNumber;
    state.email = email;
    state.errors = result.errors;

    std::string html = ResidentView::renderEditForm(residentOpt.value(), state);

    auto resp = drogon::HttpResponse::newHttpResponse();
    resp->setStatusCode(drogon::k200OK);
    resp->setContentTypeCode(drogon::CT_TEXT_HTML);
    resp->setBody(std::move(html));
    callback(resp);
}

void ResidentController::deactivateResident(
    const drogon::HttpRequestPtr&,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
    const std::string& idStr)
{
    int residentId = -1;
    try
    {
        residentId = std::stoi(idStr);
    }
    catch (...)
    {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k404NotFound);
        resp->setContentTypeCode(drogon::CT_TEXT_HTML);
        resp->setBody(ResidentView::renderNotFound("Invalid resident identifier."));
        callback(resp);
        return;
    }

    auto result = deactivationService_->deactivateResident(residentId);

    if (result.notFound)
    {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k404NotFound);
        resp->setContentTypeCode(drogon::CT_TEXT_HTML);
        resp->setBody(ResidentView::renderNotFound("The requested resident record could not be found."));
        callback(resp);
        return;
    }

    // Post/Redirect/Get pattern
    auto resp = drogon::HttpResponse::newRedirectionResponse("/residents?msg=deactivated", drogon::k303SeeOther);
    callback(resp);
}

} // namespace csms