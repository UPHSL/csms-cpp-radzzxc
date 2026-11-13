#include "ServiceRequest.h"
#include <utility>

namespace csms
{

ServiceRequest::ServiceRequest(
    int residentId,
    std::string serviceType,
    std::string description,
    std::string dateRequested,
    std::string status,
    std::optional<int> id
)
    : id_(id),
      residentId_(residentId),
      serviceType_(std::move(serviceType)),
      description_(std::move(description)),
      dateRequested_(std::move(dateRequested)),
      status_(std::move(status))
{
}

std::optional<int> ServiceRequest::getId() const
{
    return id_;
}

int ServiceRequest::getResidentId() const
{
    return residentId_;
}

const std::string& ServiceRequest::getServiceType() const
{
    return serviceType_;
}

const std::string& ServiceRequest::getDescription() const
{
    return description_;
}

const std::string& ServiceRequest::getDateRequested() const
{
    return dateRequested_;
}

const std::string& ServiceRequest::getStatus() const
{
    return status_;
}

void ServiceRequest::setResidentId(int residentId)
{
    residentId_ = residentId;
}

void ServiceRequest::setServiceType(const std::string& serviceType)
{
    serviceType_ = serviceType;
}

void ServiceRequest::setDescription(const std::string& description)
{
    description_ = description;
}

void ServiceRequest::setDateRequested(const std::string& dateRequested)
{
    dateRequested_ = dateRequested;
}

void ServiceRequest::setStatus(const std::string& status)
{
    status_ = status;
}

} // namespace csms