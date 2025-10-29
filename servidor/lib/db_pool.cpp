#include "db_pool.hpp"

std::shared_ptr<pqxx::connection> PgPool::open_conn(const std::string& conninfo) {
    auto c = std::make_shared<pqxx::connection>(conninfo);
    if (!c->is_open()) throw std::runtime_error("pqxx: cannot open connection");
    return c;
}

bool PgPool::is_healthy(pqxx::connection& c) {
    if (!c.is_open()) return false;
    try {
        pqxx::work tx{c};
        auto r = tx.exec("SELECT 1");
        tx.commit();
        return !r.empty();
    } catch (...) {
        return false;
    }
}

PgPool::PgPool(const PgConfig& cfg, std::size_t size_min, std::size_t size_max)
  : conninfo_(cfg.to_conninfo()), size_max_(size_max) {
    if (size_min == 0 || size_min > size_max_) throw std::invalid_argument("invalid pool sizes");
    for (std::size_t i = 0; i < size_min; ++i) {
        free_.push_back(open_conn(conninfo_));
        ++total_;
    }
}

PgPool::~PgPool() {
    std::unique_lock<std::mutex> lk(m_);
    shutting_down_ = true;
    cv_.notify_all();
    free_.clear(); // shared_ptr cierra al salir del scope
}

PgConnHandle PgPool::acquire(std::chrono::milliseconds timeout) {
    std::unique_lock<std::mutex> lk(m_);
    auto until = std::chrono::steady_clock::now() + timeout;

    while (true) {
        if (!free_.empty()) {
            auto c = free_.front(); free_.pop_front();
            lk.unlock();
            if (!is_healthy(*c)) c = open_conn(conninfo_);
            return PgConnHandle{std::move(c), this};
        }
        if (total_ < size_max_) {
            lk.unlock();
            auto c = open_conn(conninfo_);
            {
                std::lock_guard<std::mutex> g(m_);
                ++total_;
            }
            return PgConnHandle{std::move(c), this};
        }
        if (cv_.wait_until(lk, until) == std::cv_status::timeout)
            throw std::runtime_error("PgPool acquire timeout");
        if (shutting_down_) throw std::runtime_error("PgPool shutting down");
    }
}

void PgPool::release_(std::shared_ptr<pqxx::connection> c) {
    if (!c) return;
    std::lock_guard<std::mutex> lk(m_);
    if (shutting_down_) return; // dejar destruir
    free_.push_back(std::move(c));
    cv_.notify_one();
}

PgConnHandle::~PgConnHandle() { release(); }

void PgConnHandle::release() {
    if (conn_ && pool_) pool_->release_(std::move(conn_));
    conn_.reset();
    pool_ = nullptr;
}
