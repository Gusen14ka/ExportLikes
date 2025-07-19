#pragma once

#include <string>
#include <chrono>
#include <vector>
#include <functional>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio/awaitable.hpp>
#include "SpotifyIoService.hpp"


class SpotifyClient{
public:
    // Constuctor
    explicit SpotifyClient();
    ~SpotifyClient();
    // Authorize - generate Authorize url
    std::string authorize();

    // Generate token
    boost::asio::awaitable<void> fetchTokens(std::string code);

    // Read json, use searchTrack to get tracks' ids
    // Then send ids to addTracksToLibrary in batches
    boost::asio::awaitable<void> likeTracksFromJson(const std::string& jsonPath,
                                                    std::function<void(int, int)> progressCb);

    // Remove last N tracks from "Like library"
    // Get last N track from library and send them to sendRemoveReq
    boost::asio::awaitable<void> removeLastN(std::size_t n,
                                             std::function<void(int, int)> progressCb);

    // Check the validity of access token
    bool hasValidAccessToken() const;

    // Setter clientId_
    void setClientId(std::string id){ clientId_ = id; }

    // Setter redirectUri
    void setRedirectUri(std::string uri) { redirectUri_ = uri; }

    // Setter authorizeCode
    void setAuthorizationCode(std::string code) { authorizationCode_ = code; }

    // Getter authorizeCode
    std::string getAuthorizationCode() { return authorizationCode_; }

    // Getter access token
    std::string getAccessToken() { return accessToken_; }
private:
    // Send DELETE-request to spotify
    // to remove tracks "Like library" by their ids
    boost::asio::awaitable<void> sendRemoveReq(const std::vector<std::string>& ids);

    // "Like" batch of tracks by their ids
    boost::asio::awaitable<void> addTracksToLibrary(const std::vector<std::string>& trackIds);

    // Search one track by artist and title
    // Return its spotify-id
    boost::asio::awaitable<std::string> searchTrack(
        const std::string& artist, const std::string& title);

    // Encode string to URL-safety string
    std::string encodeURL(const std::string& val);

    boost::asio::ip::tcp::resolver resolver_;
    boost::asio::ssl::context ssl_ctx_;

    std::string codeVerifier_;
    std::string clientId_;
    std::string redirectUri_;

    std::string authorizationCode_;

    std::string accessToken_;
    std::string refreshToken_;
    std::chrono::steady_clock::time_point tokenExpiry_;
};
