#include "server/UserListMethod.h"

#include "server/UserManager.h"

namespace RPCServer {

UserListMethod::UserListMethod(XmlRpc::XmlRpcServer* server, std::shared_ptr<UserManager> userManager)
    : ServiceMethod("userList", "Listar usuarios", server), userManager_(std::move(userManager)) {}

void UserListMethod::execute(XmlRpc::XmlRpcValue& params, XmlRpc::XmlRpcValue& result) {
    if (params.size() != 1) {
        throw XmlRpc::XmlRpcException("userList requiere 1 parámetro: token");
    }

    std::string token = static_cast<std::string>(params[0]);

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

        auto users = userManager_->getAllUsers();

        result["success"] = true;
        result["users"].setSize(users.size());

        for (size_t i = 0; i < users.size(); ++i) {
            result["users"][i]["id"] = users[i].id;
            result["users"][i]["username"] = users[i].username;
            result["users"][i]["role"] = users[i].role;
            result["users"][i]["active"] = users[i].active;
            result["users"][i]["createdAt"] = static_cast<int>(users[i].createdAt);
        }

    } catch (const std::exception& e) {
        result["success"] = false;
        result["message"] = std::string("Error: ") + e.what();
    }
}

} // namespace RPCServer
