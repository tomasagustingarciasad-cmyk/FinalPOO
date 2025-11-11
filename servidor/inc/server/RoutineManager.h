#pragma once

#include <memory>
#include <string>
#include <vector>
#include <ctime>

#include "server/DatabaseManager.h"

namespace RPCServer {

class RoutineManager {
public:
    struct RoutineInfo {
        int id;
        std::string filename;
        std::string originalFilename;
        std::string description;
        int fileSize;
        int userId;
        std::time_t createdAt;
    };

    explicit RoutineManager(std::shared_ptr<DatabaseManager> dbMgr);

    int createRoutine(const std::string& filename, const std::string& originalFilename,
                      const std::string& description, const std::string& gcodeContent, int userId);
    DatabaseManager::Routine* getRoutineById(int routineId, int requestUserId, const std::string& userRole);
    std::vector<RoutineInfo> getAllRoutines(int requestUserId, const std::string& userRole);
    bool updateRoutine(int routineId, const std::string& filename, const std::string& description,
                       const std::string& gcodeContent, int requestUserId, const std::string& userRole);
    bool deleteRoutine(int routineId, int requestUserId, const std::string& userRole);

private:
    std::shared_ptr<DatabaseManager> dbManager_;
};

} // namespace RPCServer
