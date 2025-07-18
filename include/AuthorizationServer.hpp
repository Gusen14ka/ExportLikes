#pragma once

#include "SpotifyIoService.hpp"
#include <boost/asio.hpp>
#include <boost/asio/awaitable.hpp>

class AuthorizationServer{
public:
    explicit AuthorizationServer(unsigned short port);
    ~AuthorizationServer();

    void shoutdown();

    boost::asio::awaitable<std::string> asyncGetAuthorizationCode();
private:
    unsigned short port_;
    boost::asio::ip::tcp::acceptor acceptor_;
};
