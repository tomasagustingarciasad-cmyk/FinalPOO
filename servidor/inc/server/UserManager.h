#pragma once

#include <memory>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>
#include <ctime>

#include "server/DatabaseManager.h"

namespace RPCServer {

class UserManager {
private:
    struct Session {
        std::string token;
        int userId;
        std::time_t createdAt;
        std::time_t lastAccess;
        std::string userAgent;
        std::string ip;
    };

    std::shared_ptr<DatabaseManager> dbManager_;
    std::random_device rd_;
    std::mt19937 gen_;
    std::unordered_map<std::string, Session> sessions_;

    std::string generateToken();

public:
    struct UserInfo {
        int id;
        std::string username;
        std::string role;
        bool active;
        std::time_t createdAt;
    };

    explicit UserManager(std::shared_ptr<DatabaseManager> dbMgr);

    std::string login(const std::string& username, const std::string& password,
                      const std::string& userAgent = "", const std::string& ip = "");
    bool validateToken(const std::string& token);
    UserInfo* getUserByToken(const std::string& token);
    bool logout(const std::string& token);

    int createUser(const std::string& username, const std::string& password, const std::string& role = "OPERATOR");
    std::vector<UserInfo> getAllUsers();
    UserInfo* getUserByUsername(const std::string& username);
    bool updateUser(int userId, const std::string& newUsername, const std::string& newRole);
    bool updateUserPassword(const std::string& username, const std::string& newPassword);
    bool deleteUser(int userId);
    bool deleteUser(const std::string& username);
    void cleanExpiredSessions(int maxAgeHours = 24);
};

} // namespace RPCServer
