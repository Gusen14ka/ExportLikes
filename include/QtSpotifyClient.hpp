#pragma once

#include <QObject>
#include <QString>
#include <boost/asio/awaitable.hpp>
#include <boost/asio.hpp>
#include <QPointer>


class SpotifyClient;
class AuthorizationServer;

class QtSpotifyClient : public QObject{
    Q_OBJECT
public:
    explicit QtSpotifyClient(QObject* parent = nullptr);
    ~QtSpotifyClient() override;
public slots:
    void setClientId(const QString& id);
    void setRedirectUri(const QString& uri);
    void loadLocalJson(const QString& path);
    void addTracks();
    void removeLastNTracks(const std::size_t n);
signals:
    void logMessage(const QString& msg);
    void progress(int current, int total);
    void finishedAdding(bool success);
    void finishedRemoving(bool success);
private:
    // Safe call signals
    template <typename Func, typename... Args>
    static void safeCall(QPointer<QtSpotifyClient> self, Func&& func, Args&&... args) {
        if (self) {
            (self->*std::forward<Func>(func))(std::forward<Args>(args)...);
        }
    }

    boost::asio::awaitable<void> runAsyncAddingPipeline();
    boost::asio::awaitable<void> runAsyncRemovingPipeline(const std::size_t n);

    QString clientId_;
    QString redirectUri_;
    QString jsonPath_;
    QString authorizationCode_;

    std::unique_ptr<SpotifyClient> sp_client_;
    std::unique_ptr<AuthorizationServer> authSrv_;
};
