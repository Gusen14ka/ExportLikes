#include "exportlikes.hpp"
#include <QApplication>
/*
#include <boost/asio.hpp>
#include <QDebug>
#include <memory>

using namespace boost::asio;

class EchoSession : public std::enable_shared_from_this<EchoSession>{
public:
    EchoSession(ip::tcp::socket socket) : socket_(std::move(socket)){}
    void start(){
        doRead();
    }

private:
    void doRead(){
        auto self = shared_from_this();
        socket_.async_read_some(buffer(data_),
            [this, self](boost::system::error_code ec, std::size_t length){
                if(!ec){
                    this->doWrite(length);
                }
        });
    }
    void doWrite(std::size_t length){
        auto self = shared_from_this();
        async_write(socket_, buffer(data_, length),
            [this, self](boost::system::error_code ec, std::size_t){
                if(!ec){
                    this->doRead();
                }
        });
    }
    ip::tcp::socket socket_;
    std::array<char, 1024> data_;
};

class EchoServer{
public:
    EchoServer(io_context& context, unsigned short port)
        : acceptor_(context, ip::tcp::endpoint(ip::tcp::v4(), port)){
        qDebug() << "Server started on port " << port << "\n";
        doAccept();
    }
private:
    void doAccept(){
        acceptor_.async_accept(
            [this](boost::system::error_code ec, ip::tcp::socket socket){
                if(!ec){
                    qDebug() << "Accepted connection from \n";
                    std::make_shared<EchoSession>(std::move(socket))->start();
                }
                else{
                    qDebug() << "Accept error: " << ec.message() << "\n";
                }
                doAccept();
            });
    }
    ip::tcp::acceptor acceptor_;
};

int main(){

    try{
        // Initialize context
        io_context io;
        EchoServer server (io, 4001);
        io.run();
    }
    catch(std::exception& e){
        qDebug() << "Error: " << e.what() << "\n";
    }
}
*/

int main(int argc, char* argv[]) {
    //_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_CHECK_ALWAYS_DF | _CRTDBG_LEAK_CHECK_DF);
    QApplication a(argc, argv);
    ExportLikes w;
    w.show();
    return a.exec();
}




