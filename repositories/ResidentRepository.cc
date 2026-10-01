#include "ResidentRepository.h"
#include <stdexcept>

namespace csms
{

ResidentRepository::ResidentRepository(const std::string& dbPath)
    : dbConn_(std::make_shared<DatabaseConnection>(dbPath))
{
}

ResidentRepository::ResidentRepository(std::shared_ptr<DatabaseConnection> dbConn)
    : dbConn_(std::move(dbConn))
{
    if (!dbConn_ || !dbConn_->getRawHandle())
    {
        throw std::invalid_argument("DatabaseConnection must be valid and open.");
    }
}

// Reusable row mapper: extracts column data from SQLite statement into a Resident domain model
Resident ResidentRepository::mapResident(sqlite3_stmt* stmt) const
{
    int id = sqlite3_column_int(stmt, 0);
    std::string firstName = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
    std::string lastName = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
    std::string address = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
    std::string contactNumber = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
    std::string email = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
    std::string status = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));

    return Resident(
        firstName,
        lastName,
        address,
        contactNumber,
        email,
        status,
        id
    );
}

Resident ResidentRepository::save(const Resident& resident)
{
    sqlite3* db = dbConn_->getRawHandle();

    const char* insertSql =
        "INSERT INTO residents (first_name, last_name, address, contact_number, email, status) "
        "VALUES (?, ?, ?, ?, ?, ?);";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db, insertSql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK)
    {
        throw std::runtime_error(std::string("Failed to prepare insert statement: ") + sqlite3_errmsg(db));
    }

    sqlite3_bind_text(stmt, 1, resident.getFirstName().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, resident.getLastName().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, resident.getAddress().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, resident.getContactNumber().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, resident.getEmail().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 6, resident.getStatus().c_str(), -1, SQLITE_TRANSIENT);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE)
    {
        std::string error = sqlite3_errmsg(db);
        sqlite3_finalize(stmt);
        throw std::runtime_error("Failed to execute insert statement: " + error);
    }

    sqlite3_finalize(stmt);

    int newId = static_cast<int>(sqlite3_last_insert_rowid(db));

    return Resident(
        resident.getFirstName(),
        resident.getLastName(),
        resident.getAddress(),
        resident.getContactNumber(),
        resident.getEmail(),
        resident.getStatus(),
        newId
    );
}

std::optional<Resident> ResidentRepository::findById(int residentId)
{
    sqlite3* db = dbConn_->getRawHandle();

    const char* selectSql =
        "SELECT id, first_name, last_name, address, contact_number, email, status "
        "FROM residents WHERE id = ?;";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db, selectSql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK)
    {
        throw std::runtime_error(std::string("Failed to prepare select statement: ") + sqlite3_errmsg(db));
    }

    sqlite3_bind_int(stmt, 1, residentId);

    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW)
    {
        // Reuse mapper for consistent conversion
        Resident resident = mapResident(stmt);
        sqlite3_finalize(stmt);
        return resident;
    }

    sqlite3_finalize(stmt);
    return std::nullopt;
}

std::vector<Resident> ResidentRepository::findAll()
{
    sqlite3* db = dbConn_->getRawHandle();

    // Query all records with deterministic, case-insensitive sorting
    const char* sql =
        "SELECT id, first_name, last_name, address, contact_number, email, status "
        "FROM residents "
        "ORDER BY LOWER(last_name) ASC, LOWER(first_name) ASC, id ASC;";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK)
    {
        throw std::runtime_error(std::string("Unable to prepare Resident list query: ") + sqlite3_errmsg(db));
    }

    std::vector<Resident> residents;
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
    {
        residents.push_back(mapResident(stmt));
    }

    if (rc != SQLITE_DONE)
    {
        std::string error = sqlite3_errmsg(db);
        sqlite3_finalize(stmt);
        throw std::runtime_error("Unable to read Resident list: " + error);
    }

    sqlite3_finalize(stmt);
    return residents;
}

std::vector<Resident> ResidentRepository::searchByName(const std::string& searchTerm)
{
    sqlite3* db = dbConn_->getRawHandle();

    // Search matches either first_name or last_name using SQLite parameter binding.
    // Query performs filtering directly in the database without in-memory looping.
    const char* sql =
        "SELECT id, first_name, last_name, address, contact_number, email, status "
        "FROM residents "
        "WHERE LOWER(first_name) LIKE LOWER(?) OR LOWER(last_name) LIKE LOWER(?) "
        "ORDER BY LOWER(last_name) ASC, LOWER(first_name) ASC, id ASC;";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK)
    {
        throw std::runtime_error(std::string("Unable to prepare Resident search query: ") + sqlite3_errmsg(db));
    }

    // Build wildcard pattern: %searchTerm%
    const std::string pattern = "%" + searchTerm + "%";

    // Bind pattern safely to both placeholders (parameter 1 = first_name, parameter 2 = last_name)
    sqlite3_bind_text(stmt, 1, pattern.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, pattern.c_str(), -1, SQLITE_TRANSIENT);

    std::vector<Resident> residents;
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
    {
        residents.push_back(mapResident(stmt));
    }

    if (rc != SQLITE_DONE)
    {
        std::string error = sqlite3_errmsg(db);
        sqlite3_finalize(stmt);
        throw std::runtime_error("Unable to search Residents: " + error);
    }

    sqlite3_finalize(stmt);
    return residents;
}

bool ResidentRepository::update(const Resident& resident)
{
    // A resident must have an assigned ID to be updated
    if (!resident.getId().has_value())
    {
        throw std::invalid_argument("Cannot update a resident without an assigned ID.");
    }

    sqlite3* db = dbConn_->getRawHandle();

    // SQL query modifies only the permitted fields for the targeted ID
    // id and status are explicitly excluded from the SET clause
    const char* updateSql =
        "UPDATE residents "
        "SET first_name = ?, last_name = ?, address = ?, contact_number = ?, email = ? "
        "WHERE id = ?;";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db, updateSql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK)
    {
        throw std::runtime_error(std::string("Failed to prepare update statement: ") + sqlite3_errmsg(db));
    }

    // Safely bind permitted editable values using parameter binding
    sqlite3_bind_text(stmt, 1, resident.getFirstName().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, resident.getLastName().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, resident.getAddress().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, resident.getContactNumber().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, resident.getEmail().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 6, resident.getId().value());

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE)
    {
        std::string error = sqlite3_errmsg(db);
        sqlite3_finalize(stmt);
        throw std::runtime_error("Failed to execute update statement: " + error);
    }

    sqlite3_finalize(stmt);

    // sqlite3_changes reports how many rows were actually modified
    return sqlite3_changes(db) > 0;
}

bool ResidentRepository::deactivateById(int residentId)
{
    sqlite3* db = dbConn_->getRawHandle();

    // SQL query modifies only the status field for the targeted ID
    // ID, personal details, and contact number are preserved
    const char* sql = "UPDATE residents SET status = ? WHERE id = ?;";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK)
    {
        throw std::runtime_error(std::string("Failed to prepare deactivation statement: ") + sqlite3_errmsg(db));
    }

    sqlite3_bind_text(stmt, 1, "Inactive", -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, residentId);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE)
    {
        std::string error = sqlite3_errmsg(db);
        sqlite3_finalize(stmt);
        throw std::runtime_error("Failed to execute deactivation statement: " + error);
    }

    sqlite3_finalize(stmt);

    return sqlite3_changes(db) > 0;
}

} // namespace csms

