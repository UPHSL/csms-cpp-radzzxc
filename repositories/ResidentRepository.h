#pragma once

#include <string>
#include <optional>
#include <vector>
#include <memory>
#include <sqlite3.h>
#include "models/Resident.h"
#include "database/DatabaseConnection.h"

namespace csms
{

class ResidentRepository
{
public:
    // Can be constructed with a dbPath or with a shared DatabaseConnection
    explicit ResidentRepository(const std::string& dbPath = "data/csms.db");
    explicit ResidentRepository(std::shared_ptr<DatabaseConnection> dbConn);

    Resident save(const Resident& resident);
    std::optional<Resident> findById(int residentId);

    // T05: Query all persisted residents ordered by lastName ASC, firstName ASC, id ASC
    std::vector<Resident> findAll();

    // T05: Case-insensitive partial search on firstName or lastName with same deterministic ordering
    std::vector<Resident> searchByName(const std::string& searchTerm);

private:
    std::shared_ptr<DatabaseConnection> dbConn_;

    // Helper: Maps the current row of an executed SQLite statement into a Resident domain entity
    Resident mapResident(sqlite3_stmt* stmt) const;
};

} // namespace csms
