#pragma once

#include <string>
#include <optional>
#include <memory>
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

private:
    std::shared_ptr<DatabaseConnection> dbConn_;
};

} // namespace csms
