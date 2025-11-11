#include "server/IsConnectedMethod.h"

#include "Robot.h"

namespace RPCServer {

IsConnectedMethod::IsConnectedMethod(XmlRpc::XmlRpcServer* server, Robot* robot)
    : ServiceMethod("isConnected", "Indica si el robot está conectado", server), robot_(robot) {}

void IsConnectedMethod::execute(XmlRpc::XmlRpcValue& /*params*/, XmlRpc::XmlRpcValue& result) {
    try {
        bool connected = robot_->isConnected();
        result["success"] = true;
        result["connected"] = connected;
    } catch (const std::exception& e) {
        result["success"] = false;
        result["message"] = std::string("Error: ") + e.what();
    }
}

} // namespace RPCServer
