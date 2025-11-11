#include "server/RoutineManager.h"

namespace RPCServer {

RoutineManager::RoutineManager(std::shared_ptr<DatabaseManager> dbMgr)
    : dbManager_(std::move(dbMgr)) {}

int RoutineManager::createRoutine(const std::string& filename, const std::string& originalFilename,
                                  const std::string& description, const std::string& gcodeContent, int userId) {
    if (!dbManager_) {
        return -1;
    }
    return dbManager_->createRoutine(filename, originalFilename, description, gcodeContent, userId);
}

DatabaseManager::Routine* RoutineManager::getRoutineById(int routineId, int requestUserId, const std::string& userRole) {
    if (!dbManager_) {
        return nullptr;
    }

    auto routine = dbManager_->getRoutineById(routineId);
    if (!routine) {
        return nullptr;
    }

    if (userRole == "ADMIN" || routine->userId == requestUserId) {
        return routine;
    }

    return nullptr;
}

std::vector<RoutineManager::RoutineInfo> RoutineManager::getAllRoutines(int requestUserId, const std::string& userRole) {
    std::vector<RoutineInfo> result;
    if (!dbManager_) {
        return result;
    }

    std::vector<DatabaseManager::Routine> routines;

    if (userRole == "ADMIN") {
        routines = dbManager_->getAllRoutines();
    } else {
        routines = dbManager_->getRoutinesByUser(requestUserId);
    }

    for (const auto& routine : routines) {
        RoutineInfo info;
        info.id = routine.id;
        info.filename = routine.filename;
        info.originalFilename = routine.originalFilename;
        info.description = routine.description;
        info.fileSize = routine.fileSize;
        info.userId = routine.userId;
        info.createdAt = routine.createdAt;
        result.push_back(info);
    }

    return result;
}

bool RoutineManager::updateRoutine(int routineId, const std::string& filename, const std::string& description,
                                   const std::string& gcodeContent, int requestUserId, const std::string& userRole) {
    if (!dbManager_) {
        return false;
    }

    auto routine = dbManager_->getRoutineById(routineId);
    if (!routine) {
        return false;
    }

    if (userRole != "ADMIN" && routine->userId != requestUserId) {
        return false;
    }

    return dbManager_->updateRoutine(routineId, filename, description, gcodeContent);
}

bool RoutineManager::deleteRoutine(int routineId, int requestUserId, const std::string& userRole) {
    if (!dbManager_) {
        return false;
    }

    auto routine = dbManager_->getRoutineById(routineId);
    if (!routine) {
        return false;
    }

    if (userRole != "ADMIN" && routine->userId != requestUserId) {
        return false;
    }

    return dbManager_->deleteRoutine(routineId);
}

} // namespace RPCServer
