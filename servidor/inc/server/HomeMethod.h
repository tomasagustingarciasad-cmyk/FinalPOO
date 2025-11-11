#pragma once

#include "server/ServiceMethod.h"

namespace RPCServer {

class Robot;

class HomeMethod : public ServiceMethod {
private:
    Robot* robot_;

public:
    HomeMethod(XmlRpc::XmlRpcServer* server, Robot* robot);
    void execute(XmlRpc::XmlRpcValue& params, XmlRpc::XmlRpcValue& result) override;
};

} // namespace RPCServer
