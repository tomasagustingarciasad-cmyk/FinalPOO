#include "server/ServiceMethod.h"

#include <sstream>

#include "XmlRpcException.h"

namespace RPCServer {

ServiceMethod::ServiceMethod(const std::string& name, const std::string& description, XmlRpc::XmlRpcServer* server)
    : XmlRpc::XmlRpcServerMethod(name, server), methodDescription(description) {
    LOG_SYSTEM("ServiceMethod", LogLevel::DEBUG, "Método registrado", "Method: " + name + " - " + description);
}

void ServiceMethod::logRequest(const std::string& user, const std::string& clientIP, int responseCode, const std::string& details) {
    LOG_REQUEST(_name, user, clientIP, responseCode, details);
}

std::string ServiceMethod::getClientIP() {
    // En una implementación real, esto debería extraer la IP del contexto de la conexión
    return "127.0.0.1";
}

std::string ServiceMethod::help() {
    return methodDescription;
}

} // namespace RPCServer
