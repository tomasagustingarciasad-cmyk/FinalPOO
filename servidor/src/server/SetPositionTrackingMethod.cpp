#include "server/SetPositionTrackingMethod.h"

#include "Robot.h"

namespace RPCServer {

SetPositionTrackingMethod::SetPositionTrackingMethod(XmlRpc::XmlRpcServer* server, Robot* robot)
    : ServiceMethod("setPositionTracking", "Habilitar/deshabilitar tracking de posición", server), robot_(robot) {}

void SetPositionTrackingMethod::execute(XmlRpc::XmlRpcValue& params, XmlRpc::XmlRpcValue& result) {
    if (params.size() != 1) {
        throw XmlRpc::XmlRpcException("setPositionTracking requiere 1 parámetro: enable:bool");
    }

    try {
        if (!robot_->isConnected()) {
            result["success"] = false;
            result["message"] = "Robot no conectado";
            return;
        }

        bool enable = bool(params[0]);
        robot_->enablePositionTracking(enable);

        result["success"] = true;
        result["message"] = enable ? "Tracking habilitado" : "Tracking deshabilitado";
        result["trackingEnabled"] = enable;

    } catch (const std::exception& e) {
        result["success"] = false;
        result["message"] = std::string("Error: ") + e.what();
    }
}

} // namespace RPCServer
