#include "server/DatabaseManager.h"

#include <iostream>
#include <pqxx/pqxx>

namespace RPCServer {

DatabaseManager::DatabaseManager() {
    PgConfig cfg;
    cfg.host = "localhost";
    cfg.port = 31432;
    cfg.dbname = "poo";
    cfg.user = "postgres";
    cfg.password = "pass123";
    cfg.ssl_disable = true;

    try {
        dbPool_ = std::make_shared<PgPool>(cfg, 2, 10);
        std::cout << "DatabaseManager: Conectado a PostgreSQL" << std::endl;
        createDefaultUsers();
    } catch (const std::exception& e) {
        std::cerr << "ERROR DatabaseManager: " << e.what() << std::endl;
        dbPool_ = nullptr;
    }
}

bool DatabaseManager::isConnected() const {
    return dbPool_ != nullptr;
}

std::string DatabaseManager::hashPassword(const std::string& password) {
    return password;
}

void DatabaseManager::createDefaultUsers() {
    if (!dbPool_) {
        return;
    }

    try {
        auto handle = dbPool_->acquire();
        pqxx::work txn(handle.conn());

        auto adminResult = txn.exec_params(
            "SELECT usuario_id FROM finalpoo.usuario WHERE username = $1", "admin");

        if (adminResult.empty()) {
            txn.exec_params(
                "INSERT INTO finalpoo.usuario (username, password_hash, role) VALUES ($1, hash_password($2), $3)",
                "admin", "Admin123", "ADMIN");
        }

        auto userResult = txn.exec_params(
            "SELECT usuario_id FROM finalpoo.usuario WHERE username = $1", "user");

        if (userResult.empty()) {
            txn.exec_params(
                "INSERT INTO finalpoo.usuario (username, password_hash, role) VALUES ($1, hash_password($2), $3)",
                "user", "User123", "OPERATOR");
        }

        txn.commit();
    } catch (const std::exception& e) {
        std::cerr << "Error creando usuarios por defecto: " << e.what() << std::endl;
    }
}

int DatabaseManager::createUser(const std::string& username, const std::string& password, const std::string& role) {
    if (!dbPool_) {
        return -1;
    }

    try {
        auto handle = dbPool_->acquire();
        pqxx::work txn(handle.conn());

        auto result = txn.exec_params(
            "SELECT usuario_id FROM finalpoo.usuario WHERE username = $1", username);

        if (!result.empty()) {
            return -1;
        }

        auto insertResult = txn.exec_params(
            "INSERT INTO finalpoo.usuario (username, password_hash, role) "
            "VALUES ($1, hash_password($2), $3) RETURNING usuario_id",
            username, password, role);

        txn.commit();

        if (!insertResult.empty()) {
            return insertResult[0][0].as<int>();
        }
        return -1;

    } catch (const std::exception& e) {
        std::cerr << "Error creating user: " << e.what() << std::endl;
        return -1;
    }
}

DatabaseManager::User* DatabaseManager::authenticateUser(const std::string& username, const std::string& password) {
    if (!dbPool_) {
        return nullptr;
    }

    try {
        auto handle = dbPool_->acquire();
        pqxx::work txn(handle.conn());

        auto result = txn.exec_params(
            "SELECT usuario_id, username, role, active, "
            "EXTRACT(EPOCH FROM created_at)::INTEGER as created_at "
            "FROM finalpoo.usuario WHERE username = $1 AND password_hash = hash_password($2) AND active = true",
            username, password);

        if (result.empty()) {
            return nullptr;
        }

        static User user;
        user.id = result[0][0].as<int>();
        user.username = result[0][1].as<std::string>();
        user.role = result[0][2].as<std::string>();
        user.active = result[0][3].as<bool>();
        user.createdAt = static_cast<std::time_t>(result[0][4].as<int>());

        return &user;

    } catch (const std::exception& e) {
        std::cerr << "Error authenticating user: " << e.what() << std::endl;
        return nullptr;
    }
}

DatabaseManager::User* DatabaseManager::getUserByUsername(const std::string& username) {
    if (!dbPool_) {
        return nullptr;
    }

    try {
        auto handle = dbPool_->acquire();
        pqxx::work txn(handle.conn());

        auto result = txn.exec_params(
            "SELECT usuario_id, username, role, active, "
            "EXTRACT(EPOCH FROM created_at)::INTEGER as created_at "
            "FROM finalpoo.usuario WHERE username = $1",
            username);

        if (result.empty()) {
            return nullptr;
        }

        static User user;
        user.id = result[0][0].as<int>();
        user.username = result[0][1].as<std::string>();
        user.role = result[0][2].as<std::string>();
        user.active = result[0][3].as<bool>();
        user.createdAt = static_cast<std::time_t>(result[0][4].as<int>());

        return &user;

    } catch (const std::exception& e) {
        std::cerr << "Error getting user by username: " << e.what() << std::endl;
        return nullptr;
    }
}

DatabaseManager::User* DatabaseManager::getUserById(int userId) {
    if (!dbPool_) {
        return nullptr;
    }

    try {
        auto handle = dbPool_->acquire();
        pqxx::work txn(handle.conn());

        auto result = txn.exec_params(
            "SELECT usuario_id, username, role, active, "
            "EXTRACT(EPOCH FROM created_at)::INTEGER as created_at "
            "FROM finalpoo.usuario WHERE usuario_id = $1",
            userId);

        if (result.empty()) {
            return nullptr;
        }

        static User user;
        user.id = result[0][0].as<int>();
        user.username = result[0][1].as<std::string>();
        user.role = result[0][2].as<std::string>();
        user.active = result[0][3].as<bool>();
        user.createdAt = static_cast<std::time_t>(result[0][4].as<int>());

        return &user;

    } catch (const std::exception& e) {
        std::cerr << "Error getting user by ID: " << e.what() << std::endl;
        return nullptr;
    }
}

std::vector<DatabaseManager::User> DatabaseManager::getAllUsers() {
    std::vector<User> result;
    if (!dbPool_) {
        return result;
    }

    try {
        auto handle = dbPool_->acquire();
        pqxx::work txn(handle.conn());

        auto queryResult = txn.exec(
            "SELECT usuario_id, username, role, active, "
            "EXTRACT(EPOCH FROM created_at)::INTEGER as created_at "
            "FROM finalpoo.usuario ORDER BY created_at DESC");

        for (const auto& row : queryResult) {
            User user;
            user.id = row[0].as<int>();
            user.username = row[1].as<std::string>();
            user.role = row[2].as<std::string>();
            user.active = row[3].as<bool>();
            user.createdAt = static_cast<std::time_t>(row[4].as<int>());
            result.push_back(user);
        }

    } catch (const std::exception& e) {
        std::cerr << "Error getting all users: " << e.what() << std::endl;
    }

    return result;
}

bool DatabaseManager::updateUser(int userId, const std::string& newUsername, const std::string& newRole) {
    if (!dbPool_) {
        return false;
    }

    try {
        auto handle = dbPool_->acquire();
        pqxx::work txn(handle.conn());

        auto result = txn.exec_params(
            "UPDATE finalpoo.usuario SET username = $1, role = $2 WHERE usuario_id = $3",
            newUsername, newRole, userId);

        txn.commit();
        return result.affected_rows() > 0;

    } catch (const std::exception& e) {
        std::cerr << "Error updating user: " << e.what() << std::endl;
        return false;
    }
}

bool DatabaseManager::updateUserPassword(const std::string& username, const std::string& newPassword) {
    if (!dbPool_) {
        return false;
    }

    try {
        auto handle = dbPool_->acquire();
        pqxx::work txn(handle.conn());

        auto result = txn.exec_params(
            "UPDATE finalpoo.usuario SET password_hash = hash_password($1) WHERE username = $2",
            newPassword, username);

        txn.commit();
        return result.affected_rows() > 0;

    } catch (const std::exception& e) {
        std::cerr << "Error updating user password: " << e.what() << std::endl;
        return false;
    }
}

bool DatabaseManager::deleteUser(int userId) {
    if (!dbPool_) {
        return false;
    }

    try {
        auto handle = dbPool_->acquire();
        pqxx::work txn(handle.conn());

        auto userCheck = txn.exec_params(
            "SELECT username FROM finalpoo.usuario WHERE usuario_id = $1", userId);

        if (!userCheck.empty() && userCheck[0][0].as<std::string>() == "admin") {
            return false;
        }

        auto result = txn.exec_params(
            "DELETE FROM finalpoo.usuario WHERE usuario_id = $1", userId);

        txn.commit();
        return result.affected_rows() > 0;

    } catch (const std::exception& e) {
        std::cerr << "Error deleting user: " << e.what() << std::endl;
        return false;
    }
}

int DatabaseManager::createRoutine(const std::string& filename, const std::string& originalFilename,
                                   const std::string& description, const std::string& gcodeContent, int userId) {
    if (!dbPool_) {
        return -1;
    }

    try {
        auto handle = dbPool_->acquire();
        pqxx::work txn(handle.conn());

        auto result = txn.exec_params(
            "INSERT INTO finalpoo.gcode_routine (filename, original_filename, description, gcode_content, file_size, user_id) "
            "VALUES ($1, $2, $3, $4, $5, $6) RETURNING routine_id",
            filename, originalFilename, description, gcodeContent, static_cast<int>(gcodeContent.size()), userId);

        txn.commit();

        if (!result.empty()) {
            return result[0][0].as<int>();
        }
        return -1;

    } catch (const std::exception& e) {
        std::cerr << "Error creating routine: " << e.what() << std::endl;
        return -1;
    }
}

DatabaseManager::Routine* DatabaseManager::getRoutineById(int routineId) {
    if (!dbPool_) {
        return nullptr;
    }

    try {
        auto handle = dbPool_->acquire();
        pqxx::work txn(handle.conn());

        auto result = txn.exec_params(
            "SELECT routine_id, filename, original_filename, description, gcode_content, file_size, user_id, "
            "EXTRACT(EPOCH FROM created_at)::INTEGER as created_at "
            "FROM finalpoo.gcode_routine WHERE routine_id = $1",
            routineId);

        if (result.empty()) {
            return nullptr;
        }

        static Routine routine;
        routine.id = result[0][0].as<int>();
        routine.filename = result[0][1].as<std::string>();
        routine.originalFilename = result[0][2].as<std::string>();
        routine.description = result[0][3].as<std::string>();
        routine.gcodeContent = result[0][4].as<std::string>();
        routine.fileSize = result[0][5].as<int>();
        routine.userId = result[0][6].as<int>();
        routine.createdAt = static_cast<std::time_t>(result[0][7].as<int>());

        return &routine;

    } catch (const std::exception& e) {
        std::cerr << "Error getting routine by ID: " << e.what() << std::endl;
        return nullptr;
    }
}

std::vector<DatabaseManager::Routine> DatabaseManager::getAllRoutines() {
    std::vector<Routine> result;
    if (!dbPool_) {
        return result;
    }

    try {
        auto handle = dbPool_->acquire();
        pqxx::work txn(handle.conn());

        auto queryResult = txn.exec(
            "SELECT routine_id, filename, original_filename, description, gcode_content, file_size, user_id, "
            "EXTRACT(EPOCH FROM created_at)::INTEGER as created_at "
            "FROM finalpoo.gcode_routine ORDER BY created_at DESC");

        for (const auto& row : queryResult) {
            Routine routine;
            routine.id = row[0].as<int>();
            routine.filename = row[1].as<std::string>();
            routine.originalFilename = row[2].as<std::string>();
            routine.description = row[3].as<std::string>();
            routine.gcodeContent = row[4].as<std::string>();
            routine.fileSize = row[5].as<int>();
            routine.userId = row[6].as<int>();
            routine.createdAt = static_cast<std::time_t>(row[7].as<int>());
            result.push_back(routine);
        }

    } catch (const std::exception& e) {
        std::cerr << "Error getting all routines: " << e.what() << std::endl;
    }

    return result;
}

std::vector<DatabaseManager::Routine> DatabaseManager::getRoutinesByUser(int userId) {
    std::vector<Routine> result;
    if (!dbPool_) {
        return result;
    }

    try {
        auto handle = dbPool_->acquire();
        pqxx::work txn(handle.conn());

        auto queryResult = txn.exec_params(
            "SELECT routine_id, filename, original_filename, description, gcode_content, file_size, user_id, "
            "EXTRACT(EPOCH FROM created_at)::INTEGER as created_at "
            "FROM finalpoo.gcode_routine WHERE user_id = $1 ORDER BY created_at DESC",
            userId);

        for (const auto& row : queryResult) {
            Routine routine;
            routine.id = row[0].as<int>();
            routine.filename = row[1].as<std::string>();
            routine.originalFilename = row[2].as<std::string>();
            routine.description = row[3].as<std::string>();
            routine.gcodeContent = row[4].as<std::string>();
            routine.fileSize = row[5].as<int>();
            routine.userId = row[6].as<int>();
            routine.createdAt = static_cast<std::time_t>(row[7].as<int>());
            result.push_back(routine);
        }

    } catch (const std::exception& e) {
        std::cerr << "Error getting routines by user: " << e.what() << std::endl;
    }

    return result;
}

bool DatabaseManager::updateRoutine(int routineId, const std::string& filename, const std::string& description, const std::string& gcodeContent) {
    if (!dbPool_) {
        return false;
    }

    try {
        auto handle = dbPool_->acquire();
        pqxx::work txn(handle.conn());

        auto result = txn.exec_params(
            "UPDATE finalpoo.gcode_routine SET filename = $1, description = $2, gcode_content = $3, file_size = $4 "
            "WHERE routine_id = $5",
            filename, description, gcodeContent, static_cast<int>(gcodeContent.size()), routineId);

        txn.commit();
        return result.affected_rows() > 0;

    } catch (const std::exception& e) {
        std::cerr << "Error updating routine: " << e.what() << std::endl;
        return false;
    }
}

bool DatabaseManager::deleteRoutine(int routineId) {
    if (!dbPool_) {
        return false;
    }

    try {
        auto handle = dbPool_->acquire();
        pqxx::work txn(handle.conn());

        auto result = txn.exec_params(
            "DELETE FROM finalpoo.gcode_routine WHERE routine_id = $1", routineId);

        txn.commit();
        return result.affected_rows() > 0;

    } catch (const std::exception& e) {
        std::cerr << "Error deleting routine: " << e.what() << std::endl;
        return false;
    }
}

} // namespace RPCServer
