#pragma once

#include <memory>
#include <string>

#include "server/ServiceMethod.h"

namespace RPCServer {

class UserManager;

class UserInfoMethod : public ServiceMethod {
private:
    std::shared_ptr<UserManager> userManager_;

public:
    UserInfoMethod(XmlRpc::XmlRpcServer* server, std::shared_ptr<UserManager> userManager);
    void execute(XmlRpc::XmlRpcValue& params, XmlRpc::XmlRpcValue& result) override;
};

} // namespace RPCServer
