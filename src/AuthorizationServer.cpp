#include "AuthorizationServer.hpp"

#include<boost/beast.hpp>
#include<QDebug>
#include <chrono>

using namespace boost::asio;
using namespace boost::beast;

AuthorizationServer::AuthorizationServer(unsigned short port) :
    port_(port)
    , acceptor_{GlobalIoService::instance()}
{
    using namespace boost::asio;
    using namespace boost::asio::ip;

    qDebug() << "Creating AuthorizationServer on port" << port_;

    try {
        tcp::endpoint endpoint(tcp::v4(), port_);

        // Open and prepare acceptor
        acceptor_.open(endpoint.protocol());
        acceptor_.set_option(tcp::acceptor::reuse_address(true));
        acceptor_.bind(endpoint);
        acceptor_.listen();

        qDebug() << "Acceptor is ready on port" << port_;
    } catch (const std::exception& e) {
        qCritical() << "Failed to start AuthorizationServer: " << e.what();
        throw;
    }
}

AuthorizationServer::~AuthorizationServer(){
    qDebug() << "Destructor server";
}

void AuthorizationServer::shoutdown() {
    boost::system::error_code ec;
    //acceptor_.cancel(ec);
    acceptor_.close(ec);
    if (ec) {
        qWarning() << "Error in shoutdown: " << ec.what();
    }
}

awaitable<std::string> AuthorizationServer::asyncGetAuthorizationCode(){
    try{
        qDebug() << "[AuthSrv] Enter asyncGetAuthorizationCode";
        auto ex = co_await this_coro::executor;
        auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(60);
        steady_timer cancel_timer{ex};
        while(true){
            // Compute time to wait
            auto now = std::chrono::steady_clock::now();
            if(now >= deadline){
                qWarning() << "[AuthSrv] Timeout exceeded, aborting";
                co_return "";
            }

            cancel_timer.expires_after(deadline - now);
            co_spawn(
                ex,
                [ct = &cancel_timer, &acceptor = acceptor_]() -> awaitable<void>{
                    error_code ec;
                    co_await ct->async_wait(use_awaitable);
                    acceptor.cancel(ec);
                    co_return;
                }(),
                detached
            );

            // Listen one imcoming connetion
            qDebug() << "[AuthSrv] Acceptor constructed, waiting for accept...";
            ip::tcp::socket socket = co_await acceptor_.async_accept(use_awaitable);
            cancel_timer.cancel();


            qDebug() << "[AuthSrv] Accepted connection from"
                     << QString::fromStdString(socket.remote_endpoint().address().to_string());

            // Read HTTP-request
            flat_buffer buf;
            qDebug() << "[AuthSrv] About to async_read request...";
            http::request<http::string_body> req;
            co_await http::async_read(socket, buf, req, use_awaitable);
            qDebug() << "[AuthSrv] Request read, target =" << QString::fromStdString(req.target());

            // Parse code from target
            std::string target = std::string(req.target());
            qDebug() << "[AuthSrv] Parsing code from target";
            std::string code;
            auto pos = target.find("code=");
            if (pos != std::string::npos) {
                std::string code = target.substr(pos + 5);
                if (auto amp = code.find('&'); amp != std::string::npos)
                    code.resize(amp);

                qDebug() << "[AuthSrv] Parsed code =" << QString::fromStdString(code);

                http::response<http::string_body> res{http::status::ok, req.version()};
                res.set(http::field::content_type, "text/html");
                res.body() = "<h2>Authorization complete. You can close this window.</h2>";
                res.prepare_payload();
                co_await http::async_write(socket, res, use_awaitable);

                co_return code;
            }
            else{
                qWarning() << "Error: there is not code in spotify request";

                // Send 404 to Spotify and next iteration
                http::response<http::string_body> res{http::status::not_found, req.version()};
                res.set(http::field::content_type, "text/plain");
                res.body() = "Not Found";
                res.prepare_payload();
                co_await http::async_write(socket, res, use_awaitable);
            }
        }

    }
    catch (const boost::system::system_error& e) {
        if (e.code() == boost::asio::error::operation_aborted) {
            qWarning() << "[AuthSrv] accept() was cancelled by timer — timeout.";
            co_return "";
        }
        qWarning() << "[AuthSrv] System error:" << e.what();
        co_return "";
    }
    catch(std::exception& e){
        qWarning() << "Error in asyncGetAuthorizationCode:" << e.what();
    }
    co_return "";

}
