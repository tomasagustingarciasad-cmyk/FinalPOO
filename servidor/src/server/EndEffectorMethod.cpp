#include "server/EndEffectorMethod.h"

#include "Robot.h"
#include "RPCExceptions.h"

namespace RPCServer {

EndEffectorMethod::EndEffectorMethod(XmlRpc::XmlRpcServer* server, Robot* robot)
    : ServiceMethod("endEffector", "Control gripper/endeffector", server), robot_(robot) {}

void EndEffectorMethod::execute(XmlRpc::XmlRpcValue& params, XmlRpc::XmlRpcValue& result) {
    if (params.size() < 1) {
        throw InvalidParametersException("endEffector", "on:bool");
    }

    try {
        if (!robot_->isConnected()) {
            result["ok"] = false;
            result["message"] = "No conectado";
            return;
        }

        const bool on = static_cast<bool>(params[0]);
        const bool ok = robot_->endEffector(on);
        result["ok"] = ok;
        result["message"] = ok ? (on ? "Gripper ON" : "Gripper OFF") : "Fallo endEffector";
    } catch (const std::exception& e) {
        throw MethodExecutionException("endEffector", e.what());
    }
}

} // namespace RPCServer
