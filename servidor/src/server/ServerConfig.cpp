#include "server/ServerConfig.h"

namespace RPCServer {

ServerConfig::ServerConfig(int serverPort, bool enableIntrospection, int verbosity)
    : port(serverPort), introspectionEnabled(enableIntrospection), verbosityLevel(verbosity) {}

int ServerConfig::getPort() const {
    return port;
}

bool ServerConfig::isIntrospectionEnabled() const {
    return introspectionEnabled;
}

int ServerConfig::getVerbosityLevel() const {
    return verbosityLevel;
}

void ServerConfig::setPort(int newPort) {
    port = newPort;
}

void ServerConfig::setIntrospectionEnabled(bool enabled) {
    introspectionEnabled = enabled;
}

void ServerConfig::setVerbosityLevel(int level) {
    verbosityLevel = level;
}

} // namespace RPCServer
