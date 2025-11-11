#pragma once

namespace RPCServer {

class ServerConfig {
private:
    int port;
    bool introspectionEnabled;
    int verbosityLevel;

public:
    ServerConfig(int serverPort = 8080, bool enableIntrospection = true, int verbosity = 5);

    int getPort() const;
    bool isIntrospectionEnabled() const;
    int getVerbosityLevel() const;

    void setPort(int newPort);
    void setIntrospectionEnabled(bool enabled);
    void setVerbosityLevel(int level);
};

} // namespace RPCServer
