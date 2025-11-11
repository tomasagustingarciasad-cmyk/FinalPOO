#include "server/UserInfoMethod.h"

#include "server/UserManager.h"

namespace RPCServer {

UserInfoMethod::UserInfoMethod(XmlRpc::XmlRpcServer* server, std::shared_ptr<UserManager> userManager)
    : ServiceMethod("userInfo", "Información de usuario", server), userManager_(std::move(userManager)) {}

void UserInfoMethod::execute(XmlRpc::XmlRpcValue& params, XmlRpc::XmlRpcValue& result) {
    if (params.size() != 2) {
        throw XmlRpc::XmlRpcException("userInfo requiere 2 parámetros: token, username");
    }

    std::string token = static_cast<std::string>(params[0]);
    std::string username = static_cast<std::string>(params[1]);

    try {
        if (!userManager_->validateToken(token)) {
            result["success"] = false;
            result["message"] = "Token inválido";
            return;
        }

        auto userInfo = userManager_->getUserByUsername(username);
        if (!userInfo) {
            result["success"] = false;
            result["message"] = "Usuario no encontrado";
            return;
        }

        result["success"] = true;
        result["user"]["id"] = userInfo->id;
        result["user"]["username"] = userInfo->username;
        result["user"]["role"] = userInfo->role;
        result["user"]["active"] = userInfo->active;
        result["user"]["createdAt"] = static_cast<int>(userInfo->createdAt);

    } catch (const std::exception& e) {
        result["success"] = false;
        result["message"] = std::string("Error: ") + e.what();
    }
}

} // namespace RPCServer
