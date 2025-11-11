#include "server/GetPositionMethod.h"

#include "Robot.h"

namespace RPCServer {

GetPositionMethod::GetPositionMethod(XmlRpc::XmlRpcServer* server, Robot* robot)
    : ServiceMethod("getPosition", "Obtener posición actual del robot", server), robot_(robot) {}

void GetPositionMethod::execute(XmlRpc::XmlRpcValue& /*params*/, XmlRpc::XmlRpcValue& result) {
    try {
        if (!robot_->isConnected()) {
            result["success"] = false;
            result["message"] = "Robot no conectado";
            return;
        }

        Position pos = robot_->getCurrentPosition();

        result["success"] = true;
        result["position"]["x"] = pos.x;
        result["position"]["y"] = pos.y;
        result["position"]["z"] = pos.z;
        result["position"]["feedrate"] = pos.feedrate;
        result["position"]["endEffectorActive"] = pos.endEffectorActive;

    } catch (const std::exception& e) {
        result["success"] = false;
        result["message"] = std::string("Error: ") + e.what();
    }
}

} // namespace RPCServer
