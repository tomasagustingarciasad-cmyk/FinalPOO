#pragma once
#include <pqxx/pqxx>
#include <string>
#include <deque>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <memory>
#include <stdexcept>

struct PgConfig {
    std::string host = "localhost";
    int         port = 5432;
    std::string dbname = "poo";
    std::string user = "postgres";
    std::string password = "pass123";
    bool        ssl_disable = true;
    int         connect_timeout_sec = 5;

    std::string to_conninfo() const {
        std::string ci = "host=" + host +
                         " port=" + std::to_string(port) +
                         " dbname=" + dbname +
                         " user=" + user +
                         " password=" + password +
                         " connect_timeout=" + std::to_string(connect_timeout_sec);
        if (ssl_disable) ci += " sslmode=disable";
        return ci;
    }
};

class PgPool;

class PgConnHandle {
public:
    PgConnHandle(std::shared_ptr<pqxx::connection> c, PgPool* p)
      : conn_(std::move(c)), pool_(p) {}
    PgConnHandle(PgConnHandle&& o) noexcept { swap(o); }
    PgConnHandle& operator=(PgConnHandle&& o) noexcept {
        if (this != &o) { release(); swap(o); }
        return *this;
    }
    PgConnHandle(const PgConnHandle&) = delete;
    PgConnHandle& operator=(const PgConnHandle&) = delete;
    ~PgConnHandle();

    pqxx::connection& conn() { return *conn_; }

private:
    void release();
    void swap(PgConnHandle& o) noexcept { std::swap(conn_, o.conn_); std::swap(pool_, o.pool_); }

    std::shared_ptr<pqxx::connection> conn_{};
    PgPool* pool_{nullptr};
};

class PgPool {
public:
    PgPool(const PgConfig& cfg, std::size_t size_min, std::size_t size_max);
    ~PgPool();

    PgConnHandle acquire(std::chrono::milliseconds timeout = std::chrono::seconds(5));

private:
    friend class PgConnHandle;
    void release_(std::shared_ptr<pqxx::connection> c);

    static std::shared_ptr<pqxx::connection> open_conn(const std::string& conninfo);
    static bool is_healthy(pqxx::connection& c);

    const std::string conninfo_;
    const std::size_t size_max_;

    std::mutex m_;
    std::condition_variable cv_;
    std::deque<std::shared_ptr<pqxx::connection>> free_;
    std::size_t total_{0};
    bool shutting_down_{false};
};
