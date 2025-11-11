#pragma once

#include "server/ServiceMethod.h"

namespace RPCServer {

class Robot;

class ConnectRobotMethod : public ServiceMethod {
private:
    Robot* robot_;

public:
    ConnectRobotMethod(XmlRpc::XmlRpcServer* server, Robot* robot);
    void execute(XmlRpc::XmlRpcValue& params, XmlRpc::XmlRpcValue& result) override;
};

} // namespace RPCServer
