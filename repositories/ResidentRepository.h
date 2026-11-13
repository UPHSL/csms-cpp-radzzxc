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

    // T05: Listing & search
    std::vector<Resident> findAll();
    std::vector<Resident> searchByName(const std::string& searchTerm);

    // T06: Updates an existing resident record in SQLite by ID.
    // Returns true if a record was updated, false if no matching row existed.
    bool update(const Resident& resident);

    // T07: Soft-deactivates an existing resident record by setting status = 'Inactive'.
    // Preserves id, personal, and contact information. Returns true if a row was updated.
    bool deactivateById(int residentId);

    // T14: Aggregate queries for the dashboard. SQLite does the counting (COUNT(*)),
    // so no Resident rows are loaded into memory.
    // Counts every persisted resident, Active and Inactive alike.
    int countAll();
    // Counts persisted residents whose status equals the given value (e.g. "Active").
    int countByStatus(const std::string& status);

private:
    std::shared_ptr<DatabaseConnection> dbConn_;

    // Helper: Maps the current row of an executed SQLite statement into a Resident domain entity
    Resident mapResident(sqlite3_stmt* stmt) const;
};

} // namespace csms
