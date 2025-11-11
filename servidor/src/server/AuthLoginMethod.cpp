#include "server/AuthLoginMethod.h"

#include "server/UserManager.h"

namespace RPCServer {

AuthLoginMethod::AuthLoginMethod(XmlRpc::XmlRpcServer* server, std::shared_ptr<UserManager> userManager)
    : ServiceMethod("authLogin", "Autenticación de usuarios", server), userManager_(std::move(userManager)) {}

void AuthLoginMethod::execute(XmlRpc::XmlRpcValue& params, XmlRpc::XmlRpcValue& result) {
    std::string clientIP = getClientIP();
    std::string username = "unknown";

    if (params.size() != 2) {
        LOG_WARNING("AuthLoginMethod", "Intento de login con parámetros incorrectos");
        logRequest("unknown", clientIP, 400, "Invalid parameters count");
        throw XmlRpc::XmlRpcException("authLogin requiere 2 parámetros: username, password");
    }

    username = static_cast<std::string>(params[0]);
    std::string password = static_cast<std::string>(params[1]);

    LOG_DEBUG("AuthLoginMethod", "execute", "Intento de login", "Usuario: " + username);

    try {
        std::string token = userManager_->login(username, password);

        if (token.empty()) {
            LOG_WARNING("AuthLoginMethod", "Login fallido - credenciales inválidas para usuario: " + username);
            logRequest(username, clientIP, 401, "Invalid credentials");
            result["success"] = false;
            result["message"] = "Credenciales inválidas";
            return;
        }

        auto userInfo = userManager_->getUserByToken(token);
        if (!userInfo) {
            LOG_ERROR("AuthLoginMethod", "TOKEN_ERROR", "Error obteniendo usuario por token", "Token: " + token);
            logRequest(username, clientIP, 500, "Internal server error");
            result["success"] = false;
            result["message"] = "Error interno del servidor";
            return;
        }

        LOG_INFO("AuthLoginMethod", "Login exitoso para usuario: " + username + " (ID: " + std::to_string(userInfo->id) + ")");
        logRequest(username, clientIP, 200, "Login successful - Role: " + userInfo->role);

        result["success"] = true;
        result["token"] = token;
        result["user"]["id"] = userInfo->id;
        result["user"]["username"] = userInfo->username;
        result["user"]["role"] = userInfo->role;
        result["user"]["active"] = userInfo->active;

    } catch (const std::exception& e) {
        LOG_ERROR("AuthLoginMethod", "LOGIN_EXCEPTION", "Excepción durante login", "Usuario: " + username + " Error: " + e.what());
        logRequest(username, clientIP, 500, "Login exception: " + std::string(e.what()));
        result["success"] = false;
        result["message"] = std::string("Error de login: ") + e.what();
    }
}

} // namespace RPCServer
