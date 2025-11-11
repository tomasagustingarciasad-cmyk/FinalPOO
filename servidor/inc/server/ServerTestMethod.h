#pragma once

#include "server/ServiceMethod.h"

namespace RPCServer {

class ServerTestMethod : public ServiceMethod {
public:
    explicit ServerTestMethod(XmlRpc::XmlRpcServer* server);
    void execute(XmlRpc::XmlRpcValue& params, XmlRpc::XmlRpcValue& result) override;
};

} // namespace RPCServer
