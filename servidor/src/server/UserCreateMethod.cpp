#include "server/UserCreateMethod.h"

#include "server/UserManager.h"

namespace RPCServer {

UserCreateMethod::UserCreateMethod(XmlRpc::XmlRpcServer* server, std::shared_ptr<UserManager> userManager)
    : ServiceMethod("userCreate", "Crear nuevo usuario", server), userManager_(std::move(userManager)) {}

void UserCreateMethod::execute(XmlRpc::XmlRpcValue& params, XmlRpc::XmlRpcValue& result) {
    if (params.size() != 4) {
        throw XmlRpc::XmlRpcException("userCreate requiere 4 parámetros: token, username, password, role");
    }

    std::string token = static_cast<std::string>(params[0]);
    std::string username = static_cast<std::string>(params[1]);
    std::string password = static_cast<std::string>(params[2]);
    std::string role = static_cast<std::string>(params[3]);

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

        int userId = userManager_->createUser(username, password, role);

        if (userId > 0) {
            result["success"] = true;
            result["userId"] = userId;
            result["message"] = "Usuario creado exitosamente";
        } else {
            result["success"] = false;
            result["message"] = "Error creando usuario (posiblemente ya existe)";
        }

    } catch (const std::exception& e) {
        result["success"] = false;
        result["message"] = std::string("Error: ") + e.what();
    }
}

} // namespace RPCServer
