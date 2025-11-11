#pragma once

#include "server/ServiceMethod.h"

namespace RPCServer {

class SumMethod : public ServiceMethod {
public:
    explicit SumMethod(XmlRpc::XmlRpcServer* server);
    void execute(XmlRpc::XmlRpcValue& params, XmlRpc::XmlRpcValue& result) override;
};

} // namespace RPCServer
