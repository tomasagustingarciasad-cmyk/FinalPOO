#include "server/SetModeMethod.h"

#include "RPCExceptions.h"
#include "Robot.h"

namespace RPCServer {

SetModeMethod::SetModeMethod(XmlRpc::XmlRpcServer* server, Robot* robot)
    : ServiceMethod("setMode", "Configura modo manual/absoluto", server), robot_(robot) {}

void SetModeMethod::execute(XmlRpc::XmlRpcValue& params, XmlRpc::XmlRpcValue& result) {
    try {
        if (params.size() < 2) {
            throw InvalidParametersException("setMode", "manual:bool, absolute:bool");
        }
        if (!robot_->isConnected()) {
            result["ok"] = false;
            result["message"] = "No conectado";
            return;
        }
        bool manual = bool(params[0]);
        bool absolute = bool(params[1]);
        bool ok = robot_->setMode(manual, absolute);
        result["ok"] = ok;
        result["message"] = ok ? "OK" : "Fallo setMode";
    } catch (const std::exception& e) {
        throw MethodExecutionException("setMode", e.what());
    }
}

} // namespace RPCServer
