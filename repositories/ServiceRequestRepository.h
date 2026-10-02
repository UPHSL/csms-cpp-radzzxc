#pragma once

#include <string>
#include <optional>
#include <memory>
#include <sqlite3.h>
#include "models/ServiceRequest.h"
#include "database/DatabaseConnection.h"

namespace csms
{

class ServiceRequestRepository
{
public:
    explicit ServiceRequestRepository(const std::string& dbPath = "data/csms.db");
    explicit ServiceRequestRepository(std::shared_ptr<DatabaseConnection> dbConn);

    ServiceRequest save(const ServiceRequest& request);
    std::optional<ServiceRequest> findById(int id);

private:
    std::shared_ptr<DatabaseConnection> dbConn_;

    ServiceRequest mapServiceRequest(sqlite3_stmt* stmt) const;
};

} // namespace csms
