#include "DatabaseConnection.h"
#include <stdexcept>
#include <filesystem>

namespace csms
{

DatabaseConnection::DatabaseConnection(const std::string& dbPath)
    : dbPath_(dbPath), db_(nullptr)
{
    // Ensure parent directories exist if a path like "data/csms.db" was given
    std::filesystem::path p(dbPath_);
    if (p.has_parent_path())
    {
        std::filesystem::create_directories(p.parent_path());
    }

    int rc = sqlite3_open(dbPath_.c_str(), &db_);
    if (rc != SQLITE_OK)
    {
        std::string errMsg = sqlite3_errmsg(db_);
        close();
        throw std::runtime_error("Cannot open SQLite database at '" + dbPath_ + "': " + errMsg);
    }

    initializeSchema();
}

DatabaseConnection::~DatabaseConnection()
{
    close();
}

DatabaseConnection::DatabaseConnection(DatabaseConnection&& other) noexcept
    : dbPath_(std::move(other.dbPath_)), db_(other.db_)
{
    other.db_ = nullptr;
}

DatabaseConnection& DatabaseConnection::operator=(DatabaseConnection&& other) noexcept
{
    if (this != &other)
    {
        close();
        dbPath_ = std::move(other.dbPath_);
        db_ = other.db_;
        other.db_ = nullptr;
    }
    return *this;
}

void DatabaseConnection::close()
{
    if (db_ != nullptr)
    {
        sqlite3_close(db_);
        db_ = nullptr;
    }
}

void DatabaseConnection::initializeSchema()
{
    const char* createResidentsTableSql =
        "CREATE TABLE IF NOT EXISTS residents ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "first_name TEXT NOT NULL, "
        "last_name TEXT NOT NULL, "
        "address TEXT NOT NULL, "
        "contact_number TEXT NOT NULL, "
        "email TEXT NOT NULL, "
        "status TEXT NOT NULL DEFAULT 'Active'"
        ");";

    char* errMsg = nullptr;
    int rc = sqlite3_exec(db_, createResidentsTableSql, nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK)
    {
        std::string error = errMsg ? errMsg : "Unknown error";
        sqlite3_free(errMsg);
        throw std::runtime_error("Failed to initialize residents table: " + error);
    }
}

} // namespace csms
