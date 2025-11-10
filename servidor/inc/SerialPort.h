#pragma once
#include <string>

namespace RPCServer {
class SerialPort {
    int fd_;
    bool opened_;
    std::string port_;
    int baud_;
public:
    SerialPort();
    ~SerialPort();
    bool open(const std::string& port, int baud);
    void close();
    bool isOpen() const;
    bool writeLine(const std::string& line);
    std::string readLine(int timeoutMs = 500);
    // Flush input/output buffers. By default flushes both directions.
    void flushBuffers(bool flushInput = true, bool flushOutput = true);

private:
    bool configure(int baud);
    void setDtrRts(bool dtr, bool rts);
};
} // namespace RPCServer