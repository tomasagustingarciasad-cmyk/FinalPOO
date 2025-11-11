#include "server/HomeMethod.h"

#include "Robot.h"
#include "RPCExceptions.h"

namespace RPCServer {

HomeMethod::HomeMethod(XmlRpc::XmlRpcServer* server, Robot* robot)
    : ServiceMethod("home", "Homing del robot", server), robot_(robot) {}

void HomeMethod::execute(XmlRpc::XmlRpcValue& /*params*/, XmlRpc::XmlRpcValue& result) {
    try {
        if (!robot_->isConnected()) {
            result["ok"] = false;
            result["message"] = "No conectado";
            return;
        }

        const bool ok = robot_->home();
        result["ok"] = ok;
        result["message"] = ok ? "Home ejecutado" : "Fallo home";
    } catch (const std::exception& e) {
        throw MethodExecutionException("home", e.what());
    }
}

} // namespace RPCServer
