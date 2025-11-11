#include "server/DisconnectRobotMethod.h"

#include "RPCExceptions.h"
#include "Robot.h"

namespace RPCServer {

DisconnectRobotMethod::DisconnectRobotMethod(XmlRpc::XmlRpcServer* server, Robot* robot)
    : ServiceMethod("disconnectRobot", "Desconecta el puerto serie", server), robot_(robot) {}

void DisconnectRobotMethod::execute(XmlRpc::XmlRpcValue& /*params*/, XmlRpc::XmlRpcValue& result) {
    try {
        robot_->disconnect();
        result["ok"] = true;
        result["message"] = "Desconectado";
    } catch (const std::exception& e) {
        throw MethodExecutionException("disconnectRobot", e.what());
    }
}

} // namespace RPCServer
