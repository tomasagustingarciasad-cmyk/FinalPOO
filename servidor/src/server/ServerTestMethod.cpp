#include "server/ServerTestMethod.h"
#include "RPCExceptions.h"

namespace RPCServer {

ServerTestMethod::ServerTestMethod(XmlRpc::XmlRpcServer* server)
    : ServiceMethod("ServerTest", "Respondo quien soy cuando no hay argumentos", server) {}

void ServerTestMethod::execute(XmlRpc::XmlRpcValue& /*params*/, XmlRpc::XmlRpcValue& result) {
    try {
        LOG_DEBUG("ServerTestMethod", "execute", "Procesando test del servidor", "");
        result = "Hi, soy el servidor RPC !!";
        logRequest("anonymous", getClientIP(), 200, "Server test successful");
    } catch (const std::exception& e) {
        LOG_ERROR("ServerTestMethod", "EXEC_ERROR", "Error en ServerTest", e.what());
        logRequest("anonymous", getClientIP(), 500, "Server test failed: " + std::string(e.what()));
        throw MethodExecutionException("ServerTest", e.what());
    }
}

} // namespace RPCServer
