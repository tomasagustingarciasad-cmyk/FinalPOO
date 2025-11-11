#pragma once

#include <memory>
#include <string>

#include "server/ServiceMethod.h"

namespace RPCServer {

class UserManager;

class UserUpdateMethod : public ServiceMethod {
private:
    std::shared_ptr<UserManager> userManager_;

public:
    UserUpdateMethod(XmlRpc::XmlRpcServer* server, std::shared_ptr<UserManager> userManager);
    void execute(XmlRpc::XmlRpcValue& params, XmlRpc::XmlRpcValue& result) override;
};

} // namespace RPCServer
