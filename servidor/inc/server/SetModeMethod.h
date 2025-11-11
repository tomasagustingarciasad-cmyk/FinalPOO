#pragma once

#include "server/ServiceMethod.h"

namespace RPCServer {

class Robot;

class SetModeMethod : public ServiceMethod {
private:
    Robot* robot_;

public:
    SetModeMethod(XmlRpc::XmlRpcServer* server, Robot* robot);
    void execute(XmlRpc::XmlRpcValue& params, XmlRpc::XmlRpcValue& result) override;
};

} // namespace RPCServer
