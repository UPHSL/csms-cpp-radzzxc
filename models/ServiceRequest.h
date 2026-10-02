#pragma once

#include <optional>
#include <string>

namespace csms
{

class ServiceRequest
{
public:
    ServiceRequest(
        int residentId,
        std::string serviceType,
        std::string description,
        std::string dateRequested,
        std::string status = "Pending",
        std::optional<int> id = std::nullopt
    );

    std::optional<int> getId() const;
    int getResidentId() const;
    const std::string& getServiceType() const;
    const std::string& getDescription() const;
    const std::string& getDateRequested() const;
    const std::string& getStatus() const;

    void setResidentId(int residentId);
    void setServiceType(const std::string& serviceType);
    void setDescription(const std::string& description);
    void setDateRequested(const std::string& dateRequested);
    void setStatus(const std::string& status);

private:
    std::optional<int> id_;
    int residentId_;
    std::string serviceType_;
    std::string description_;
    std::string dateRequested_;
    std::string status_;
};

} // namespace csms