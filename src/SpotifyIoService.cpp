#include "SpotifyIoService.hpp"
#include <QDebug>
#include <thread>

struct GlobalIoService::Impl {
    boost::asio::io_context io_ctx;
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type> work_guard;
    std::thread io_thread;

    Impl()
        : work_guard(boost::asio::make_work_guard(io_ctx))
    {
        io_thread = std::thread([this] {
            try {
                io_ctx.run();
            } catch (...) {
                qWarning() << "IO thread exception";
            }
        });
    }

    ~Impl() {
        work_guard.reset();
        io_ctx.stop();

        if (io_thread.joinable()) {
            try {
                io_thread.join();
            } catch (...) {
                // Ignore
            }
        }
    }
};

std::unique_ptr<GlobalIoService::Impl> GlobalIoService::impl_;

boost::asio::io_context& GlobalIoService::instance() {
    if (!impl_) {
        impl_ = std::make_unique<Impl>();
    }
    return impl_->io_ctx;
}

void GlobalIoService::start() {
    // Initialize on first call
    instance();
}

void GlobalIoService::stop() {
    impl_.reset();
}
