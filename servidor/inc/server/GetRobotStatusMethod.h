#pragma once

#include "Robot.h"
#include "server/ServiceMethod.h"

namespace RPCServer {

class GetRobotStatusMethod : public ServiceMethod {
private:
    Robot* robot_;
    Position posActual;

public:
    GetRobotStatusMethod(XmlRpc::XmlRpcServer* server, Robot* robot);
    void execute(XmlRpc::XmlRpcValue& params, XmlRpc::XmlRpcValue& result) override;
};

} // namespace RPCServer
