#pragma once

#include <string>
#include <vector>
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
    bool updateStatus(int id, const std::string& newStatus);
    std::vector<ServiceRequest> findAll();
    std::vector<ServiceRequest> search(const std::string& searchTerm, const std::optional<std::string>& statusFilter = std::nullopt);

    // T14: Aggregate queries for the dashboard. SQLite does the counting (COUNT(*)),
    // so no Service Request rows are loaded into memory.
    // Counts every persisted service request, whatever its status.
    int countAll();
    // Counts persisted service requests whose current status equals the given value (e.g. "Pending").
    int countByStatus(const std::string& status);

    // T14: The newest `limit` service requests, using the same T11 ordering as findAll()
    // (date_requested DESC, then id DESC) with SQL LIMIT. limit <= 0 returns an empty list.
    std::vector<ServiceRequest> findRecent(int limit);

private:
    std::shared_ptr<DatabaseConnection> dbConn_;

    ServiceRequest mapServiceRequest(sqlite3_stmt* stmt) const;
};

} // namespace csms
