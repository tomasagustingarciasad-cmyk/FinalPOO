#pragma once

#include <memory>
#include <string>
#include <vector>
#include <ctime>
#include "db_pool.hpp"

namespace RPCServer {

class DatabaseManager {
public:
    struct User {
        int id;
        std::string username;
        std::string role;
        bool active;
        std::time_t createdAt;
    };

    struct Routine {
        int id;
        std::string filename;
        std::string originalFilename;
        std::string description;
        std::string gcodeContent;
        int fileSize;
        int userId;
        std::time_t createdAt;
    };

    DatabaseManager();

    bool isConnected() const;

    void createDefaultUsers();
    int createUser(const std::string& username, const std::string& password, const std::string& role = "OPERATOR");
    User* authenticateUser(const std::string& username, const std::string& password);
    User* getUserByUsername(const std::string& username);
    User* getUserById(int userId);
    std::vector<User> getAllUsers();
    bool updateUser(int userId, const std::string& newUsername, const std::string& newRole);
    bool updateUserPassword(const std::string& username, const std::string& newPassword);
    bool deleteUser(int userId);

    int createRoutine(const std::string& filename, const std::string& originalFilename,
                      const std::string& description, const std::string& gcodeContent, int userId);
    Routine* getRoutineById(int routineId);
    std::vector<Routine> getAllRoutines();
    std::vector<Routine> getRoutinesByUser(int userId);
    bool updateRoutine(int routineId, const std::string& filename, const std::string& description, const std::string& gcodeContent);
    bool deleteRoutine(int routineId);

private:
    std::shared_ptr<PgPool> dbPool_;

    std::string hashPassword(const std::string& password);
};

} // namespace RPCServer
