#pragma once

#include <boost/asio.hpp>
#include <memory>

class GlobalIoService {
public:
    static boost::asio::io_context& instance();
    static void start();
    static void stop();

private:
    GlobalIoService() = delete;
    ~GlobalIoService() = delete;

    struct Impl;
    static std::unique_ptr<Impl> impl_;
};
