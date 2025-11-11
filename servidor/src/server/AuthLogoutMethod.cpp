#include "server/AuthLogoutMethod.h"

#include "server/UserManager.h"

namespace RPCServer {

AuthLogoutMethod::AuthLogoutMethod(XmlRpc::XmlRpcServer* server, std::shared_ptr<UserManager> userManager)
    : ServiceMethod("authLogout", "Logout de usuarios", server), userManager_(std::move(userManager)) {}

void AuthLogoutMethod::execute(XmlRpc::XmlRpcValue& params, XmlRpc::XmlRpcValue& result) {
    if (params.size() != 1) {
        throw XmlRpc::XmlRpcException("authLogout requiere 1 parámetro: token");
    }

    std::string token = static_cast<std::string>(params[0]);

    try {
        bool success = userManager_->logout(token);
        result["success"] = success;
        result["message"] = success ? "Logout exitoso" : "Token no encontrado";
    } catch (const std::exception& e) {
        result["success"] = false;
        result["message"] = std::string("Error de logout: ") + e.what();
    }
}

} // namespace RPCServer
