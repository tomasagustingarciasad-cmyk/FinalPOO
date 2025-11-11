#pragma once

#include "server/ServiceMethod.h"

namespace RPCServer {

class EchoMethod : public ServiceMethod {
public:
    explicit EchoMethod(XmlRpc::XmlRpcServer* server);
    void execute(XmlRpc::XmlRpcValue& params, XmlRpc::XmlRpcValue& result) override;
};

} // namespace RPCServer
