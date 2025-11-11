#pragma once

#include "server/ServiceMethod.h"

namespace RPCServer {

class Robot;

class EndEffectorMethod : public ServiceMethod {
private:
    Robot* robot_;

public:
    EndEffectorMethod(XmlRpc::XmlRpcServer* server, Robot* robot);
    void execute(XmlRpc::XmlRpcValue& params, XmlRpc::XmlRpcValue& result) override;
};

} // namespace RPCServer
