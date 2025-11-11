#include "server/GetRobotStatusMethod.h"

#include "Robot.h"

namespace RPCServer {

GetRobotStatusMethod::GetRobotStatusMethod(XmlRpc::XmlRpcServer* server, Robot* robot)
    : ServiceMethod("getRobotStatus", "Obtiene estado completo del robot", server), robot_(robot) {}

void GetRobotStatusMethod::execute(XmlRpc::XmlRpcValue& /*params*/, XmlRpc::XmlRpcValue& result) {
    try {
        bool connected = robot_->isConnected();
        result["success"] = true;
        result["connected"] = connected;

        if (connected) {
            result["motorsOn"] = robot_->getMotorsOn();
            result["gripperOn"] = robot_->getGripperOn();

            try {
                posActual = robot_->getCurrentPosition();
                result["position"]["x"] = posActual.x;
                result["position"]["y"] = posActual.y;
                result["position"]["z"] = posActual.z;
            } catch (...) {
                result["position"]["x"] = 0.0;
                result["position"]["y"] = 0.0;
                result["position"]["z"] = 0.0;
            }
        } else {
            result["motorsOn"] = false;
            result["gripperOn"] = false;
            result["position"]["x"] = 0.0;
            result["position"]["y"] = 0.0;
            result["position"]["z"] = 0.0;
        }
    } catch (const std::exception& e) {
        result["success"] = false;
        result["message"] = std::string("Error: ") + e.what();
    }
}

} // namespace RPCServer
