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
        int id = sqlite3_column_int(stmt, 0);
        std::string firstName = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        std::string lastName = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        std::string address = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        std::string contactNumber = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        std::string email = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        std::string status = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));

        sqlite3_finalize(stmt);

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

    sqlite3_finalize(stmt);
    return std::nullopt;
}

} // namespace csms
