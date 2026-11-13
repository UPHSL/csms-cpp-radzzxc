#include "ServiceRequestRepository.h"
#include <stdexcept>

namespace csms
{

namespace
{

// T11 ordering rule (newest date first, then highest id). Shared by listing, search,
// and the T14 recent-requests query so they can never drift apart.
const std::string kNewestFirst = "ORDER BY date_requested DESC, id DESC";

// T14: Runs a single-value COUNT query, optionally binding one text parameter
int runCountQuery(sqlite3* db, const char* sql, const std::string* textParam)
{
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK)
    {
        throw std::runtime_error(std::string("Unable to prepare ServiceRequest count query: ") + sqlite3_errmsg(db));
    }

    if (textParam != nullptr)
    {
        sqlite3_bind_text(stmt, 1, textParam->c_str(), -1, SQLITE_TRANSIENT);
    }

    int count = 0;
    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW)
    {
        count = sqlite3_column_int(stmt, 0);
    }
    else if (rc != SQLITE_DONE)
    {
        std::string error = sqlite3_errmsg(db);
        sqlite3_finalize(stmt);
        throw std::runtime_error("Failed to execute ServiceRequest count query: " + error);
    }

    sqlite3_finalize(stmt);
    return count;
}

} // namespace

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

bool ServiceRequestRepository::updateStatus(int id, const std::string& newStatus)
{
    sqlite3* db = dbConn_->getRawHandle();

    const char* sql = "UPDATE service_requests SET status = ? WHERE id = ?;";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK)
    {
        throw std::runtime_error(std::string("Failed to prepare update status statement: ") + sqlite3_errmsg(db));
    }

    sqlite3_bind_text(stmt, 1, newStatus.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, id);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE)
    {
        std::string error = sqlite3_errmsg(db);
        sqlite3_finalize(stmt);
        throw std::runtime_error("Failed to execute update status statement: " + error);
    }

    sqlite3_finalize(stmt);

    return sqlite3_changes(db) > 0;
}

std::vector<ServiceRequest> ServiceRequestRepository::findAll()
{
    sqlite3* db = dbConn_->getRawHandle();

    const std::string sql =
        "SELECT id, resident_id, service_type, description, date_requested, status "
        "FROM service_requests " + kNewestFirst + ";";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK)
    {
        throw std::runtime_error(std::string("Unable to prepare ServiceRequest list query: ") + sqlite3_errmsg(db));
    }

    std::vector<ServiceRequest> requests;
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
    {
        requests.push_back(mapServiceRequest(stmt));
    }

    sqlite3_finalize(stmt);
    return requests;
}

std::vector<ServiceRequest> ServiceRequestRepository::search(const std::string& searchTerm, const std::optional<std::string>& statusFilter)
{
    sqlite3* db = dbConn_->getRawHandle();

    std::string sql;
    bool hasText = !searchTerm.empty();
    bool hasStatus = statusFilter.has_value() && !statusFilter.value().empty();

    if (!hasText && !hasStatus)
    {
        return findAll();
    }

    if (hasText && hasStatus)
    {
        sql = "SELECT id, resident_id, service_type, description, date_requested, status "
              "FROM service_requests "
              "WHERE (LOWER(service_type) LIKE LOWER(?) OR LOWER(description) LIKE LOWER(?)) "
              "  AND status = ? "
              + kNewestFirst + ";";
    }
    else if (hasText && !hasStatus)
    {
        sql = "SELECT id, resident_id, service_type, description, date_requested, status "
              "FROM service_requests "
              "WHERE (LOWER(service_type) LIKE LOWER(?) OR LOWER(description) LIKE LOWER(?)) "
              + kNewestFirst + ";";
    }
    else // !hasText && hasStatus
    {
        sql = "SELECT id, resident_id, service_type, description, date_requested, status "
              "FROM service_requests "
              "WHERE status = ? "
              + kNewestFirst + ";";
    }

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK)
    {
        throw std::runtime_error(std::string("Unable to prepare ServiceRequest search query: ") + sqlite3_errmsg(db));
    }

    if (hasText && hasStatus)
    {
        std::string pattern = "%" + searchTerm + "%";
        sqlite3_bind_text(stmt, 1, pattern.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, pattern.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 3, statusFilter.value().c_str(), -1, SQLITE_TRANSIENT);
    }
    else if (hasText && !hasStatus)
    {
        std::string pattern = "%" + searchTerm + "%";
        sqlite3_bind_text(stmt, 1, pattern.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, pattern.c_str(), -1, SQLITE_TRANSIENT);
    }
    else // !hasText && hasStatus
    {
        sqlite3_bind_text(stmt, 1, statusFilter.value().c_str(), -1, SQLITE_TRANSIENT);
    }

    std::vector<ServiceRequest> requests;
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
    {
        requests.push_back(mapServiceRequest(stmt));
    }

    sqlite3_finalize(stmt);
    return requests;
}

int ServiceRequestRepository::countAll()
{
    return runCountQuery(dbConn_->getRawHandle(), "SELECT COUNT(*) FROM service_requests;", nullptr);
}

int ServiceRequestRepository::countByStatus(const std::string& status)
{
    return runCountQuery(dbConn_->getRawHandle(), "SELECT COUNT(*) FROM service_requests WHERE status = ?;", &status);
}

std::vector<ServiceRequest> ServiceRequestRepository::findRecent(int limit)
{
    std::vector<ServiceRequest> requests;
    if (limit <= 0)
    {
        return requests;
    }

    sqlite3* db = dbConn_->getRawHandle();

    const std::string sql =
        "SELECT id, resident_id, service_type, description, date_requested, status "
        "FROM service_requests " + kNewestFirst + " LIMIT ?;";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK)
    {
        throw std::runtime_error(std::string("Unable to prepare recent ServiceRequest query: ") + sqlite3_errmsg(db));
    }

    sqlite3_bind_int(stmt, 1, limit);

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
    {
        requests.push_back(mapServiceRequest(stmt));
    }

    sqlite3_finalize(stmt);
    return requests;
}

} // namespace csms
