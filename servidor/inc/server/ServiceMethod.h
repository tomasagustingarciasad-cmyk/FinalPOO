#pragma once

#include <string>
#include "XmlRpc.h"
#include "CSVLogger.h"

namespace RPCServer {

class ServiceMethod : public XmlRpc::XmlRpcServerMethod {
protected:
    std::string methodDescription;

    void logRequest(const std::string& user, const std::string& clientIP, int responseCode, const std::string& details = "");
    std::string getClientIP();

public:
    ServiceMethod(const std::string& name, const std::string& description, XmlRpc::XmlRpcServer* server);
    ~ServiceMethod() override = default;

    std::string help() override;
    void execute(XmlRpc::XmlRpcValue& params, XmlRpc::XmlRpcValue& result) override = 0;
};

} // namespace RPCServer
