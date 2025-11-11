#pragma once

#include <memory>
#include <vector>

namespace XmlRpc {
class XmlRpcServer;
}

namespace RPCServer {

class ServerConfig;
class ServiceMethod;
class Robot;
class DatabaseManager;
class UserManager;

class ServerModel {
public:
    explicit ServerModel(std::unique_ptr<ServerConfig> config);
    ~ServerModel();

    void start();
    void run();
    void stop();

    int getPort() const;
    bool getIsRunning() const;

    Robot* getRobot();
    XmlRpc::XmlRpcServer* getServer();

private:
    std::unique_ptr<ServerConfig> config_;
    std::unique_ptr<XmlRpc::XmlRpcServer> server_;
    std::unique_ptr<Robot> robot_;
    std::shared_ptr<DatabaseManager> databaseManager_;
    std::shared_ptr<UserManager> userManager_;
    std::vector<std::unique_ptr<ServiceMethod>> methods_;
    bool isRunning_;

    void initializeCoreMethods();
    void initializeRobotMethods();
    void initializeAuthMethods();
};

} // namespace RPCServer
