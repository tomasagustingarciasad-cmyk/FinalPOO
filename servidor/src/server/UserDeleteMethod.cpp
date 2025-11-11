#include "server/UserDeleteMethod.h"

#include "server/UserManager.h"

namespace RPCServer {

UserDeleteMethod::UserDeleteMethod(XmlRpc::XmlRpcServer* server, std::shared_ptr<UserManager> userManager)
    : ServiceMethod("userDelete", "Eliminar usuario", server), userManager_(std::move(userManager)) {}

void UserDeleteMethod::execute(XmlRpc::XmlRpcValue& params, XmlRpc::XmlRpcValue& result) {
    if (params.size() != 2) {
        throw XmlRpc::XmlRpcException("userDelete requiere 2 parámetros: token, username");
    }

    std::string token = static_cast<std::string>(params[0]);
    std::string username = static_cast<std::string>(params[1]);

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
            result["message"] = "No se puede eliminar el usuario admin";
            return;
        }

        bool success = userManager_->deleteUser(username);

        result["success"] = success;
        result["message"] = success ? "Usuario eliminado exitosamente" : "Error eliminando usuario";

    } catch (const std::exception& e) {
        result["success"] = false;
        result["message"] = std::string("Error: ") + e.what();
    }
}

} // namespace RPCServer
