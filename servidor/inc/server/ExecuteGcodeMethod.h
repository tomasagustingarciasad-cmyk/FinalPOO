#pragma once

#include "server/ServiceMethod.h"

namespace RPCServer {

class Robot;

class ExecuteGcodeMethod : public ServiceMethod {
private:
    Robot* robot_;

public:
    ExecuteGcodeMethod(XmlRpc::XmlRpcServer* server, Robot* robot);
    void execute(XmlRpc::XmlRpcValue& params, XmlRpc::XmlRpcValue& result) override;
};

} // namespace RPCServer
