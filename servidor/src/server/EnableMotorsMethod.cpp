#include "server/EnableMotorsMethod.h"

#include "RPCExceptions.h"
#include "Robot.h"

namespace RPCServer {

EnableMotorsMethod::EnableMotorsMethod(XmlRpc::XmlRpcServer* server, Robot* robot)
    : ServiceMethod("enableMotors", "Enciende/Apaga motores", server), robot_(robot) {}

void EnableMotorsMethod::execute(XmlRpc::XmlRpcValue& params, XmlRpc::XmlRpcValue& result) {
    try {
        if (params.size() < 1) {
            throw InvalidParametersException("enableMotors", "on:bool");
        }
        if (!robot_->isConnected()) {
            result["ok"] = false;
            result["message"] = "No conectado";
            return;
        }
        bool on = static_cast<bool>(params[0]);
        bool ok = robot_->enableMotors(on);
        result["ok"] = ok;
        result["message"] = ok ? (on ? "Motores ON" : "Motores OFF") : "Fallo enableMotors";
    } catch (const std::exception& e) {
        throw MethodExecutionException("enableMotors", e.what());
    }
}

} // namespace RPCServer
