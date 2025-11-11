#include "server/UserManager.h"

#include <sstream>

namespace RPCServer {

UserManager::UserManager(std::shared_ptr<DatabaseManager> dbMgr)
    : dbManager_(std::move(dbMgr)), gen_(rd_()) {}

std::string UserManager::generateToken() {
    std::uniform_int_distribution<> dis(0, 15);
    std::stringstream ss;
    for (int i = 0; i < 32; ++i) {
        ss << std::hex << dis(gen_);
    }
    return ss.str();
}

std::string UserManager::login(const std::string& username, const std::string& password,
                               const std::string& userAgent, const std::string& ip) {
    if (!dbManager_) {
        return "";
    }

    auto user = dbManager_->authenticateUser(username, password);
    if (!user) {
        return "";
    }

    std::string token = generateToken();
    Session session{token, user->id, std::time(nullptr), std::time(nullptr), userAgent, ip};
    sessions_[token] = session;

    return token;
}

bool UserManager::validateToken(const std::string& token) {
    auto it = sessions_.find(token);
    if (it != sessions_.end()) {
        it->second.lastAccess = std::time(nullptr);
        return true;
    }
    return false;
}

UserManager::UserInfo* UserManager::getUserByToken(const std::string& token) {
    auto sessionIt = sessions_.find(token);
    if (sessionIt == sessions_.end()) {
        return nullptr;
    }

    auto user = dbManager_->getUserById(sessionIt->second.userId);
    if (!user) {
        return nullptr;
    }

    static UserInfo userInfo;
    userInfo.id = user->id;
    userInfo.username = user->username;
    userInfo.role = user->role;
    userInfo.active = user->active;
    userInfo.createdAt = user->createdAt;

    return &userInfo;
}

bool UserManager::logout(const std::string& token) {
    return sessions_.erase(token) > 0;
}

int UserManager::createUser(const std::string& username, const std::string& password, const std::string& role) {
    if (!dbManager_) {
        return -1;
    }
    return dbManager_->createUser(username, password, role);
}

std::vector<UserManager::UserInfo> UserManager::getAllUsers() {
    std::vector<UserInfo> result;
    if (!dbManager_) {
        return result;
    }

    auto users = dbManager_->getAllUsers();
    for (const auto& user : users) {
        UserInfo info;
        info.id = user.id;
        info.username = user.username;
        info.role = user.role;
        info.active = user.active;
        info.createdAt = user.createdAt;
        result.push_back(info);
    }

    return result;
}

UserManager::UserInfo* UserManager::getUserByUsername(const std::string& username) {
    if (!dbManager_) {
        return nullptr;
    }

    auto user = dbManager_->getUserByUsername(username);
    if (!user) {
        return nullptr;
    }

    static UserInfo userInfo;
    userInfo.id = user->id;
    userInfo.username = user->username;
    userInfo.role = user->role;
    userInfo.active = user->active;
    userInfo.createdAt = user->createdAt;

    return &userInfo;
}

bool UserManager::updateUser(int userId, const std::string& newUsername, const std::string& newRole) {
    if (!dbManager_) {
        return false;
    }
    return dbManager_->updateUser(userId, newUsername, newRole);
}

bool UserManager::updateUserPassword(const std::string& username, const std::string& newPassword) {
    if (!dbManager_) {
        return false;
    }
    return dbManager_->updateUserPassword(username, newPassword);
}

bool UserManager::deleteUser(int userId) {
    if (!dbManager_) {
        return false;
    }

    for (auto it = sessions_.begin(); it != sessions_.end();) {
        if (it->second.userId == userId) {
            it = sessions_.erase(it);
        } else {
            ++it;
        }
    }

    return dbManager_->deleteUser(userId);
}

bool UserManager::deleteUser(const std::string& username) {
    if (!dbManager_) {
        return false;
    }

    auto user = dbManager_->getUserByUsername(username);
    if (!user) {
        return false;
    }

    return deleteUser(user->id);
}

void UserManager::cleanExpiredSessions(int maxAgeHours) {
    std::time_t now = std::time(nullptr);
    std::time_t maxAge = maxAgeHours * 3600;

    for (auto it = sessions_.begin(); it != sessions_.end();) {
        if ((now - it->second.lastAccess) > maxAge) {
            it = sessions_.erase(it);
        } else {
            ++it;
        }
    }
}

} // namespace RPCServer
