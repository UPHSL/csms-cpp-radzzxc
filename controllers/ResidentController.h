#pragma once

#include <drogon/HttpController.h>
#include <memory>
#include <string>
#include "models/ResidentValidator.h"
#include "repositories/ResidentRepository.h"
#include "services/ResidentQueryService.h"
#include "services/ResidentRegistrationService.h"
#include "services/ResidentUpdateService.h"
#include "services/ResidentDeactivationService.h"

namespace csms
{

class ResidentController : public drogon::HttpController<ResidentController>
{
public:
    METHOD_LIST_BEGIN
    // GET /residents -> Lists or searches residents
    ADD_METHOD_TO(ResidentController::index, "/residents", drogon::Get);

    // GET /residents/register -> Displays registration form
    ADD_METHOD_TO(ResidentController::showRegisterForm, "/residents/register", drogon::Get);

    // POST /residents/register -> Submits registration
    ADD_METHOD_TO(ResidentController::registerResident, "/residents/register", drogon::Post);

    // GET /residents/{id}/edit -> Displays prefilled edit form
    ADD_METHOD_TO(ResidentController::showEditForm, "/residents/{1}/edit", drogon::Get);

    // POST /residents/{id}/edit -> Submits permitted updates
    ADD_METHOD_TO(ResidentController::updateResident, "/residents/{1}/edit", drogon::Post);

    // POST /residents/{id}/deactivate -> Soft deactivation
    ADD_METHOD_TO(ResidentController::deactivateResident, "/residents/{1}/deactivate", drogon::Post);
    METHOD_LIST_END

    // Default constructor uses default SQLite repository
    ResidentController();

    // Dependency-injection constructor for testing and isolation
    explicit ResidentController(std::shared_ptr<ResidentRepository> repository);

    // Action Handlers
    void index(
        const drogon::HttpRequestPtr& req,
        std::function<void(const drogon::HttpResponsePtr&)>&& callback
    );

    void showRegisterForm(
        const drogon::HttpRequestPtr& req,
        std::function<void(const drogon::HttpResponsePtr&)>&& callback
    );

    void registerResident(
        const drogon::HttpRequestPtr& req,
        std::function<void(const drogon::HttpResponsePtr&)>&& callback
    );

    void showEditForm(
        const drogon::HttpRequestPtr& req,
        std::function<void(const drogon::HttpResponsePtr&)>&& callback,
        const std::string& idStr
    );

    void updateResident(
        const drogon::HttpRequestPtr& req,
        std::function<void(const drogon::HttpResponsePtr&)>&& callback,
        const std::string& idStr
    );

    void deactivateResident(
        const drogon::HttpRequestPtr& req,
        std::function<void(const drogon::HttpResponsePtr&)>&& callback,
        const std::string& idStr
    );

private:
    std::shared_ptr<ResidentRepository> repository_;
    std::shared_ptr<ResidentValidator> validator_;
    std::unique_ptr<ResidentQueryService> queryService_;
    std::unique_ptr<ResidentRegistrationService> registrationService_;
    std::unique_ptr<ResidentUpdateService> updateService_;
    std::unique_ptr<ResidentDeactivationService> deactivationService_;
};

} // namespace csms