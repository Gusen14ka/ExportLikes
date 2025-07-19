#include "QtSpotifyClient.hpp"
#include "SpotifyClient.hpp"
#include "AuthorizationServer.hpp"
#include "SpotifyIoService.hpp"

#include <QDesktopServices>
#include <QInputDialog>
#include <QUrl>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDebug>

#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>


QtSpotifyClient::QtSpotifyClient(QObject* parent)
    : QObject(parent)
{
    qDebug() << "Creating components...";


    try {
        sp_client_ = std::make_unique<SpotifyClient>();
        qDebug() << "SpotifyClient created";
    } catch (...) {
        qWarning() << "Failed to create SpotifyClient";
    }

    try {
        authSrv_ = std::make_unique<AuthorizationServer>(8888);
        qDebug() << "AuthorizationServer created";
    } catch (const std::exception& e) {
        qWarning() << "Failed to create AuthorizationServer:" << e.what();
    }
}

QtSpotifyClient::~QtSpotifyClient(){
    try{
        // Finish corutins and join thread
        qDebug() << "Destroying QtSpotifyClient";

        authSrv_->shoutdown();
        authSrv_.reset();
        sp_client_.reset();

        qDebug() << "QtSpotifyClient destroyed";
    }
    catch(std::exception& e){
        qDebug() << "Error in destructor: " << e.what();
    }
}


void QtSpotifyClient::setClientId(const QString& id){
    clientId_ = id;
    sp_client_->setClientId(id.toStdString());
}

void QtSpotifyClient::setRedirectUri(const QString& uri){
    redirectUri_ = uri;
    sp_client_->setRedirectUri(uri.toStdString());
}

void QtSpotifyClient::loadLocalJson(const QString& path){
    jsonPath_ = path;
}

void QtSpotifyClient::addTracks(){
    if (jsonPath_.isEmpty()) {
        emit logMessage("Error: enter path to JSON.");
        emit finishedAdding(false);
        return;
    }

    if(!sp_client_->hasValidAccessToken()){
        emit reauthorization();
        authorization();
    }


    // Start corutine with pipeline
    boost::asio::co_spawn(
        GlobalIoService::instance(),
        [this]() -> boost::asio::awaitable<void>{
            QPointer<QtSpotifyClient> safeThis(this);
            try{
                safeCall(safeThis, &QtSpotifyClient::logMessage, "# Launch adding pipeline...");
                qDebug() << "About async pipeline";
                co_await runAsyncAddingPipeline();
            }
            catch(std::exception& e){
                qWarning() << "Exception in auth/add pipeline: " << e.what();
            }
            co_return;
        },
        boost::asio::detached
    );
    qDebug() << "co_spawn returned";
}

boost::asio::awaitable<void> QtSpotifyClient::runAsyncAddingPipeline(){
    QPointer<QtSpotifyClient> safeThis(this);
    qDebug() << "In runAsyncAddingPipeline";
    boost::asio::steady_timer t(co_await boost::asio::this_coro::executor, std::chrono::milliseconds(1));
    co_await t.async_wait(boost::asio::use_awaitable);

    try{
        safeCall(safeThis, &QtSpotifyClient::logMessage, "# Liking tracks from json...");
        co_await sp_client_->likeTracksFromJson(jsonPath_.toStdString(),
            [this, safeThis](int current, int total){
                safeCall(safeThis, &QtSpotifyClient::progress, current, total);
                //safeCall(safeThis, &QtSpotifyClient::logMessage, QString("Added: %1/%2")
                //    .arg(current).arg(total));
            });

        safeCall(safeThis, &QtSpotifyClient::logMessage, "Finished!");
        safeCall(safeThis, &QtSpotifyClient::finishedAdding, true);
    }
    catch(std::exception& e){
        safeCall(safeThis, &QtSpotifyClient::logMessage,
                 QString("Error in pipeline: %1").arg(e.what()));
        safeCall(safeThis, &QtSpotifyClient::finishedAdding, false);
    }
    co_return;
}


void QtSpotifyClient::removeLastNTracks(const std::size_t n){
    if(!sp_client_->hasValidAccessToken()){
        emit reauthorization();
        authorization();
    }

    // Start corutine with pipeline

    boost::asio::co_spawn(
        GlobalIoService::instance(),
        [this, n]() -> boost::asio::awaitable<void>{
            QPointer<QtSpotifyClient> safeThis(this);
            try{
                safeCall(safeThis, &QtSpotifyClient::logMessage, "# Launch removing pipeline...");
                qDebug() << "About async pipeline";
                co_await runAsyncRemovingPipeline(n);
            }
            catch(std::exception& e){
                qWarning() << "Exception in auth/remove pipeline: " << e.what();
            }
        },
        boost::asio::detached
    );
    qDebug() << "co_spawn returned";
}

boost::asio::awaitable<void> QtSpotifyClient::runAsyncRemovingPipeline(const std::size_t n){
    QPointer<QtSpotifyClient> safeThis(this);
    qDebug() << "In runAsyncRemovingPipeline";
    boost::asio::steady_timer t(co_await boost::asio::this_coro::executor, std::chrono::milliseconds(1));
    co_await t.async_wait(boost::asio::use_awaitable);

    try{
        safeCall(safeThis, &QtSpotifyClient::logMessage, "# Removing tracks from \"Liked Library\"...");
        co_await sp_client_->removeLastN(n,
            [this, safeThis](int current, int total){
                safeCall(safeThis, &QtSpotifyClient::progress, current, total);
                //safeCall(safeThis, &QtSpotifyClient::logMessage, QString("Removed: %1/%2")                                                                                            .arg(current).arg(total));
        });

        safeCall(safeThis, &QtSpotifyClient::logMessage, "Finished!");
        safeCall(safeThis, &QtSpotifyClient::finishedRemoving, true);
    }
    catch(std::exception& e){
        safeCall(safeThis, &QtSpotifyClient::logMessage,
                 QString("Error in pipeline: %1").arg(e.what()));
        safeCall(safeThis, &QtSpotifyClient::finishedRemoving, false);
    }
    co_return;
}

void QtSpotifyClient::authorization(){
    if(clientId_.isEmpty() ||
        redirectUri_.isEmpty()){
        emit logMessage("Error:: enter Client ID an Redirect Uri");
        emit finishedAuthorization(false);
        return;
    }
    emit logMessage("# Generation authorization code...");
    auto authUrl = sp_client_->authorize();
    qDebug() << "About to open browser";
    QDesktopServices::openUrl(QUrl(QString::fromStdString(authUrl)));
    qDebug() << "Browser opened, now co_spawn";

    boost::asio::co_spawn(
        GlobalIoService::instance(),
        [this]() -> boost::asio::awaitable<void>{
            QPointer<QtSpotifyClient> safeThis(this);
            try{
                safeCall(safeThis, &QtSpotifyClient::logMessage, "# Launch authorization server...");
                std::string code = co_await authSrv_->asyncGetAuthorizationCode();
                qDebug() << "About write code to client";
                safeCall(safeThis, &QtSpotifyClient::logMessage, "# Changing code to token...");
                sp_client_->setAuthorizationCode(code);
                co_await sp_client_->fetchTokens(sp_client_->getAuthorizationCode());
                safeCall(safeThis, &QtSpotifyClient::logMessage, "Authorization Finished!");
                safeCall(safeThis, &QtSpotifyClient::finishedAuthorization, true);
            }
            catch(std::exception& e){
                safeCall(safeThis, &QtSpotifyClient::logMessage,
                    QString("Error in authorization: %1").arg(e.what()));
            }
            co_return;
        },
        boost::asio::detached
    );
    qDebug() << "co_spawn returned";
}
