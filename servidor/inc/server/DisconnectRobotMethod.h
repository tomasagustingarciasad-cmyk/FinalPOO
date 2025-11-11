#pragma once

#include "server/ServiceMethod.h"

namespace RPCServer {

class Robot;

class DisconnectRobotMethod : public ServiceMethod {
private:
    Robot* robot_;

public:
    DisconnectRobotMethod(XmlRpc::XmlRpcServer* server, Robot* robot);
    void execute(XmlRpc::XmlRpcValue& params, XmlRpc::XmlRpcValue& result) override;
};

} // namespace RPCServer
