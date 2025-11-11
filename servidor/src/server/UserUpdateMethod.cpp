#include "server/UserUpdateMethod.h"

#include "server/UserManager.h"

namespace RPCServer {

UserUpdateMethod::UserUpdateMethod(XmlRpc::XmlRpcServer* server, std::shared_ptr<UserManager> userManager)
    : ServiceMethod("userUpdate", "Actualizar usuario", server), userManager_(std::move(userManager)) {}

void UserUpdateMethod::execute(XmlRpc::XmlRpcValue& params, XmlRpc::XmlRpcValue& result) {
    if (params.size() != 3) {
        throw XmlRpc::XmlRpcException("userUpdate requiere 3 parámetros: token, username, updates");
    }

    std::string token = static_cast<std::string>(params[0]);
    std::string username = static_cast<std::string>(params[1]);
    XmlRpc::XmlRpcValue updates = params[2];

    try {
        if (!userManager_->validateToken(token)) {
            result["success"] = false;
            result["message"] = "Token inválido";
            return;
        }

        auto currentUser = userManager_->getUserByToken(token);
        if (!currentUser || currentUser->role != "ADMIN") {
            result["success"] = false;
            result["message"] = "Permisos insuficientes";
            return;
        }

        if (username == "admin") {
            result["success"] = false;
            result["message"] = "No se puede modificar el usuario admin";
            return;
        }

        bool success = false;
        std::string message = "Usuario actualizado exitosamente";

        if (updates.hasMember("password")) {
            std::string newPassword = updates["password"];
            success = userManager_->updateUserPassword(username, newPassword);
            if (!success) {
                message = "Error actualizando contraseña";
            }
        }

        if (updates.hasMember("newUsername") || updates.hasMember("role")) {
            auto userInfo = userManager_->getUserByUsername(username);
            if (userInfo) {
                std::string newUsername = updates.hasMember("newUsername") ? std::string(updates["newUsername"]) : userInfo->username;
                std::string newRole = updates.hasMember("role") ? std::string(updates["role"]) : userInfo->role;

                success = userManager_->updateUser(userInfo->id, newUsername, newRole);
                if (!success) {
                    message = "Error actualizando información del usuario";
                }
            }
        }

        result["success"] = success;
        result["message"] = message;

    } catch (const std::exception& e) {
        result["success"] = false;
        result["message"] = std::string("Error: ") + e.what();
    }
}

} // namespace RPCServer
