#include "ServiceRequestRepository.h"
#include <stdexcept>

namespace csms
{

ServiceRequestRepository::ServiceRequestRepository(const std::string& dbPath)
    : dbConn_(std::make_shared<DatabaseConnection>(dbPath))
{
}

ServiceRequestRepository::ServiceRequestRepository(std::shared_ptr<DatabaseConnection> dbConn)
    : dbConn_(std::move(dbConn))
{
    if (!dbConn_ || !dbConn_->getRawHandle())
    {
        throw std::invalid_argument("DatabaseConnection must be valid and open.");
    }
}

ServiceRequest ServiceRequestRepository::mapServiceRequest(sqlite3_stmt* stmt) const
{
    int id = sqlite3_column_int(stmt, 0);
    int residentId = sqlite3_column_int(stmt, 1);
    std::string serviceType = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
    std::string description = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
    std::string dateRequested = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
    std::string status = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));

    return ServiceRequest(
        residentId,
        serviceType,
        description,
        dateRequested,
        status,
        id
    );
}

ServiceRequest ServiceRequestRepository::save(const ServiceRequest& request)
{
    sqlite3* db = dbConn_->getRawHandle();

    const char* insertSql =
        "INSERT INTO service_requests (resident_id, service_type, description, date_requested, status) "
        "VALUES (?, ?, ?, ?, ?);";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db, insertSql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK)
    {
        throw std::runtime_error(std::string("Failed to prepare insert statement: ") + sqlite3_errmsg(db));
    }

    sqlite3_bind_int(stmt, 1, request.getResidentId());
    sqlite3_bind_text(stmt, 2, request.getServiceType().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, request.getDescription().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, request.getDateRequested().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, request.getStatus().c_str(), -1, SQLITE_TRANSIENT);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE)
    {
        std::string error = sqlite3_errmsg(db);
        sqlite3_finalize(stmt);
        throw std::runtime_error("Failed to execute insert statement: " + error);
    }

    sqlite3_finalize(stmt);

    int newId = static_cast<int>(sqlite3_last_insert_rowid(db));

    return ServiceRequest(
        request.getResidentId(),
        request.getServiceType(),
        request.getDescription(),
        request.getDateRequested(),
        request.getStatus(),
        newId
    );
}

std::optional<ServiceRequest> ServiceRequestRepository::findById(int id)
{
    sqlite3* db = dbConn_->getRawHandle();

    const char* selectSql =
        "SELECT id, resident_id, service_type, description, date_requested, status "
        "FROM service_requests WHERE id = ?;";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db, selectSql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK)
    {
        throw std::runtime_error(std::string("Failed to prepare select statement: ") + sqlite3_errmsg(db));
    }

    sqlite3_bind_int(stmt, 1, id);

    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW)
    {
        ServiceRequest req = mapServiceRequest(stmt);
        sqlite3_finalize(stmt);
        return req;
    }

    sqlite3_finalize(stmt);
    return std::nullopt;
}

} // namespace csms
