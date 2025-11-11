#pragma once

#include <memory>

#include "server/ServiceMethod.h"

namespace RPCServer {

class UserManager;

class UserListMethod : public ServiceMethod {
private:
    std::shared_ptr<UserManager> userManager_;

public:
    UserListMethod(XmlRpc::XmlRpcServer* server, std::shared_ptr<UserManager> userManager);
    void execute(XmlRpc::XmlRpcValue& params, XmlRpc::XmlRpcValue& result) override;
};

} // namespace RPCServer
