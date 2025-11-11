#pragma once

#include "server/ServiceMethod.h"

namespace RPCServer {

class Robot;

class IsConnectedMethod : public ServiceMethod {
private:
    Robot* robot_;

public:
    IsConnectedMethod(XmlRpc::XmlRpcServer* server, Robot* robot);
    void execute(XmlRpc::XmlRpcValue& params, XmlRpc::XmlRpcValue& result) override;
};

} // namespace RPCServer
