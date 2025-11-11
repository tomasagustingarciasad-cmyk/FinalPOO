#pragma once

#include <memory>

#include "server/ServiceMethod.h"

namespace RPCServer {

class UserManager;

class AuthLogoutMethod : public ServiceMethod {
private:
    std::shared_ptr<UserManager> userManager_;

public:
    AuthLogoutMethod(XmlRpc::XmlRpcServer* server, std::shared_ptr<UserManager> userManager);
    void execute(XmlRpc::XmlRpcValue& params, XmlRpc::XmlRpcValue& result) override;
};

} // namespace RPCServer
