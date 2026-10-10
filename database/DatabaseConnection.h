#pragma once

#include <string>
#include <sqlite3.h>

namespace csms
{

class DatabaseConnection
{
public:
    explicit DatabaseConnection(const std::string& dbPath = "csms.db");
    ~DatabaseConnection();

    // Prevent copying to maintain unique handle ownership
    DatabaseConnection(const DatabaseConnection&) = delete;
    DatabaseConnection& operator=(const DatabaseConnection&) = delete;

    // Allow moving
    DatabaseConnection(DatabaseConnection&& other) noexcept;
    DatabaseConnection& operator=(DatabaseConnection&& other) noexcept;

    // Access the raw sqlite3 handle
    sqlite3* getRawHandle() const { return db_; }
    const std::string& getPath() const { return dbPath_; }

    // Table initialization / migrations
    void initializeSchema();

    void close();

private:
    std::string dbPath_;
    sqlite3* db_{nullptr};
};

} // namespace csms
