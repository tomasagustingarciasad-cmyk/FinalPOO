#include "SerialPort.h"
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <cstring>
#include <chrono>
#include <thread>

namespace RPCServer {

static speed_t toSpeedT(int baud) {
    switch (baud) {
        case 9600: return B9600;
        case 57600: return B57600;
        case 115200: return B115200;
        default: return B9600;
    }
}

SerialPort::SerialPort() : fd_(-1), opened_(false), baud_(0) {}
SerialPort::~SerialPort() { close(); }

bool SerialPort::open(const std::string& port, int baud) {
    close();
    fd_ = ::open(port.c_str(), O_RDWR | O_NOCTTY | O_SYNC);
    if (fd_ < 0) return false;
    port_ = port;
    baud_ = baud;
    if (!configure(baud)) {
        close();
        return false;
    }

    setDtrRts(true, true);
    opened_ = true;
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    return true;
}

void SerialPort::close() {
    if (fd_ >= 0) {
        ::close(fd_);
        fd_ = -1;
    }
    opened_ = false;
}

bool SerialPort::configure(int baud) {
    termios tty{};
    if (tcgetattr(fd_, &tty) != 0) return false;

    cfsetospeed(&tty, toSpeedT(baud));
    cfsetispeed(&tty, toSpeedT(baud));

    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;
    tty.c_cflag |= (CLOCAL | CREAD);
    tty.c_cflag &= ~(PARENB | PARODD);
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CRTSCTS;

    tty.c_iflag = IGNPAR;
    tty.c_oflag = 0;
    tty.c_lflag = 0; // raw

    tty.c_cc[VTIME] = 1; // 100ms units
    tty.c_cc[VMIN] = 0;

    tcflush(fd_, TCIFLUSH);
    return tcsetattr(fd_, TCSANOW, &tty) == 0;
}

void SerialPort::setDtrRts(bool dtr, bool rts) {
    if (fd_ < 0) return;
    int status = 0;
    ioctl(fd_, TIOCMGET, &status);
    if (dtr) status |= TIOCM_DTR; else status &= ~TIOCM_DTR;
    if (rts) status |= TIOCM_RTS; else status &= ~TIOCM_RTS;
    ioctl(fd_, TIOCMSET, &status);
}


bool SerialPort::writeLine(const std::string& line) {
    if (!opened_) return false;
    std::string out = line;
    // Asegurar terminación CRLF
    if (out.size() >= 2 && out[out.size()-2] == '\r' && out[out.size()-1] == '\n') {
        // ok
    } else if (!out.empty() && out.back() == '\n') {
        out.back() = '\r';
        out.push_back('\n');
    } else {
        out.append("\r\n");
    }
    ssize_t n = ::write(fd_, out.data(), out.size());
    // Asegurar que el driver transmitió todo
    tcdrain(fd_);
    // Pequeña pausa para no inundar a 9600 bps
    std::this_thread::sleep_for(std::chrono::milliseconds(3));
    return n == (ssize_t)out.size();
}

std::string SerialPort::readLine(int timeoutMs) {
    if (!opened_) return {};
    std::string buffer;
    char ch;
    int elapsed = 0;
    while (elapsed <= timeoutMs) {
        fd_set set;
        FD_ZERO(&set);
        FD_SET(fd_, &set);
        timeval tv{0, 50 * 1000}; // 50ms
        int rv = select(fd_ + 1, &set, nullptr, nullptr, &tv);
        elapsed += 50;
        if (rv > 0 && FD_ISSET(fd_, &set)) {
            ssize_t n = ::read(fd_, &ch, 1);
            if (n == 1) {
                if (ch == '\n') break;
                if (ch != '\r') buffer.push_back(ch);
            }
        }
    }
    return buffer;
}
bool SerialPort::isOpen() const { return opened_; }

} // namespace RPCServer