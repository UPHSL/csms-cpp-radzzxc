#include "HealthController.h"
#include "Version.h"

void HealthController::asyncHandleHttpRequest(
    const HttpRequestPtr&,
    std::function<void(const HttpResponsePtr&)>&& callback)
{
    Json::Value result;

    result["status"] = std::string(csms::Version::kStatus);
    result["application"] = std::string(csms::Version::kApplication);
    result["version"] = std::string(csms::Version::kVersion);

    auto response = HttpResponse::newHttpJsonResponse(result);

    callback(response);
}