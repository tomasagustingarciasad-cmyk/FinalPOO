#include "ServerModel.h"

#include <stdexcept>
#include <string>
#include <utility>

#include "CSVLogger.h"
#include "RPCExceptions.h"
#include "Robot.h"
#include "XmlRpc.h"
#include "XmlRpcException.h"
#include "XmlRpcServer.h"
#include "server/AuthLoginMethod.h"
#include "server/AuthLogoutMethod.h"
#include "server/ConnectRobotMethod.h"
#include "server/DatabaseManager.h"
#include "server/DisconnectRobotMethod.h"
#include "server/EchoMethod.h"
#include "server/EnableMotorsMethod.h"
#include "server/EndEffectorMethod.h"
#include "server/ExecuteGcodeMethod.h"
#include "server/GenerateGcodeFromMovementsMethod.h"
#include "server/GetPositionMethod.h"
#include "server/GetRobotStatusMethod.h"
#include "server/HomeMethod.h"
#include "server/IsConnectedMethod.h"
#include "server/MoveMethod.h"
#include "server/RoutineCreateMethod.h"
#include "server/RoutineDeleteMethod.h"
#include "server/RoutineGetMethod.h"
#include "server/RoutineListMethod.h"
#include "server/RoutineManager.h"
#include "server/RoutineUpdateMethod.h"
#include "server/ServerConfig.h"
#include "server/ServerTestMethod.h"
#include "server/SetModeMethod.h"
#include "server/SetPositionTrackingMethod.h"
#include "server/SumMethod.h"
#include "server/UserCreateMethod.h"
#include "server/UserDeleteMethod.h"
#include "server/UserInfoMethod.h"
#include "server/UserListMethod.h"
#include "server/UserManager.h"
#include "server/UserUpdateMethod.h"

namespace RPCServer {

ServerModel::ServerModel(std::unique_ptr<ServerConfig> config)
    : config_(std::move(config)),
      server_(std::make_unique<XmlRpc::XmlRpcServer>()),
      robot_(std::make_unique<Robot>()),
      databaseManager_(std::make_shared<DatabaseManager>()),
      userManager_(nullptr),
      methods_(),
      isRunning_(false) {
    userManager_ = std::make_shared<UserManager>(databaseManager_);

    initializeCoreMethods();
    initializeRobotMethods();
    initializeAuthMethods();
}

void ServerModel::start() {
    try {
        XmlRpc::setVerbosity(config_->getVerbosityLevel());
        server_->enableIntrospection(config_->isIntrospectionEnabled());

        if (!server_->bindAndListen(config_->getPort())) {
            throw ServerBindingException(config_->getPort(), "No se pudo vincular y escuchar");
        }

        isRunning_ = true;
    } catch (const std::exception& e) {
        throw ServerInitializationException("Falló el inicio del servidor: " + std::string(e.what()));
    }
}

void ServerModel::run() {
    if (!isRunning_) {
        throw ServerInitializationException("Servidor no iniciado");
    }

    try {
        server_->work(-1.0);
    } catch (const std::exception& e) {
        throw ServerInitializationException("Error de ejecución del servidor: " + std::string(e.what()));
    }
}

void ServerModel::stop() {
    isRunning_ = false;
    server_->shutdown();
}

int ServerModel::getPort() const {
    return config_->getPort();
}

bool ServerModel::getIsRunning() const {
    return isRunning_;
}

Robot* ServerModel::getRobot() {
    return robot_.get();
}

XmlRpc::XmlRpcServer* ServerModel::getServer() {
    return server_.get();
}

void ServerModel::initializeCoreMethods() {
    methods_.push_back(std::make_unique<ServerTestMethod>(server_.get()));
    methods_.push_back(std::make_unique<EchoMethod>(server_.get()));
    methods_.push_back(std::make_unique<SumMethod>(server_.get()));
}

void ServerModel::initializeRobotMethods() {
    Robot* rawRobot = robot_.get();

    methods_.push_back(std::make_unique<ConnectRobotMethod>(server_.get(), rawRobot));
    methods_.push_back(std::make_unique<DisconnectRobotMethod>(server_.get(), rawRobot));
    methods_.push_back(std::make_unique<SetModeMethod>(server_.get(), rawRobot));
    methods_.push_back(std::make_unique<EnableMotorsMethod>(server_.get(), rawRobot));
    methods_.push_back(std::make_unique<HomeMethod>(server_.get(), rawRobot));
    methods_.push_back(std::make_unique<MoveMethod>(server_.get(), rawRobot));
    methods_.push_back(std::make_unique<EndEffectorMethod>(server_.get(), rawRobot));
    methods_.push_back(std::make_unique<ExecuteGcodeMethod>(server_.get(), rawRobot));
    methods_.push_back(std::make_unique<GetPositionMethod>(server_.get(), rawRobot));
    methods_.push_back(std::make_unique<SetPositionTrackingMethod>(server_.get(), rawRobot));
    methods_.push_back(std::make_unique<IsConnectedMethod>(server_.get(), rawRobot));
    methods_.push_back(std::make_unique<GetRobotStatusMethod>(server_.get(), rawRobot));
}

void ServerModel::initializeAuthMethods() {
    if (!userManager_) {
        throw ServerInitializationException("UserManager no inicializado");
    }

    auto routineManager = std::make_shared<RoutineManager>(databaseManager_);

    methods_.push_back(std::make_unique<AuthLoginMethod>(server_.get(), userManager_));
    methods_.push_back(std::make_unique<AuthLogoutMethod>(server_.get(), userManager_));
    methods_.push_back(std::make_unique<UserCreateMethod>(server_.get(), userManager_));
    methods_.push_back(std::make_unique<UserListMethod>(server_.get(), userManager_));
    methods_.push_back(std::make_unique<UserInfoMethod>(server_.get(), userManager_));
    methods_.push_back(std::make_unique<UserUpdateMethod>(server_.get(), userManager_));
    methods_.push_back(std::make_unique<UserDeleteMethod>(server_.get(), userManager_));

    methods_.push_back(std::make_unique<RoutineCreateMethod>(server_.get(), userManager_, routineManager));
    methods_.push_back(std::make_unique<RoutineListMethod>(server_.get(), userManager_, routineManager));
    methods_.push_back(std::make_unique<RoutineGetMethod>(server_.get(), userManager_, routineManager));
    methods_.push_back(std::make_unique<RoutineUpdateMethod>(server_.get(), userManager_, routineManager));
    methods_.push_back(std::make_unique<RoutineDeleteMethod>(server_.get(), userManager_, routineManager));

    methods_.push_back(std::make_unique<GenerateGcodeFromMovementsMethod>(
        server_.get(), userManager_, routineManager, robot_.get()));
}

} // namespace RPCServer
