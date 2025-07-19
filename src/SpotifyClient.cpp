#include "SpotifyClient.hpp"
#include "HelperPKCE.hpp"
#include <sstream>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QIODevice>
#include <QString>
#include <boost/beast/ssl.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast.hpp>
#include <boost/beast/core.hpp>


SpotifyClient::SpotifyClient() :
    resolver_(GlobalIoService::instance())
    , ssl_ctx_(boost::asio::ssl::context::tls_client)
{
    // Off old protocols and compression
    ssl_ctx_.set_options(
        boost::asio::ssl::context::default_workarounds
        | boost::asio::ssl::context::no_sslv2
        | boost::asio::ssl::context::no_sslv3
        | boost::asio::ssl::context::single_dh_use
        );
    // Optionally on OpenSSL level off the compression
    SSL_CTX_set_options(
        ssl_ctx_.native_handle(),
        SSL_OP_NO_COMPRESSION
        );

    ssl_ctx_.set_default_verify_paths();

    // Load CA bundle (certificates)
    boost::system::error_code ec;
    ssl_ctx_.load_verify_file("cacert.pem", ec);
    if (ec) {
        qWarning() << "Unable to load CA bundle:" << QString::fromStdString(ec.message());
    }
}

SpotifyClient::~SpotifyClient(){
    //shoutdown();
    qDebug() << "Destructor client";
}


std::string SpotifyClient::encodeURL(const std::string& val){
    std::ostringstream encoded;
    for (unsigned char c : val){
        if (std::isalnum(c) || c == '_' ||
            c == '-' || c == '.' || c == '~'){
            encoded << c;
        }
        else{
            encoded << '%' << std::uppercase << std::hex << int(c);
        }
    }
    return encoded.str();
}

std::string SpotifyClient::authorize(){
    // Generate code verifier for PKCE
    codeVerifier_ = generateCodeVerifier();

    // Generate code challange for PKCE
    auto codeChallange = generateCodeChallange(codeVerifier_);

    // Make url
    auto scope = encodeURL("user-library-modify user-library-read");
    std::string url = "https://accounts.spotify.com/authorize?"
        "response_type=code"
        "&client_id=" + encodeURL(clientId_)
        + "&redirect_uri=" + encodeURL(redirectUri_)
        + "&scope=" + scope
        + "&code_challenge_method=S256"
        + "&code_challenge=" + encodeURL(codeChallange);

    return url;
}

static void configure_stream(boost::beast::ssl_stream<boost::beast::tcp_stream>& stream, char const* host)
{
    stream.set_verify_mode(boost::asio::ssl::verify_peer);

#ifdef DEBUG
    // только для отладки:
    stream.set_verify_callback(
        [](bool pre, ssl::verify_context& ctx) {
            X509_STORE_CTX* c = ctx.native_handle();
            int depth = X509_STORE_CTX_get_error_depth(c);
            int err   = X509_STORE_CTX_get_error(c);
            qDebug() << "SSL verify: depth=" << depth
                     << "err=" << err
                     << "(" << X509_verify_cert_error_string(err) << ")"
                     << "pre=" << pre;
            return pre;
        }
        );
#endif

    // SNI
    if (!SSL_set_tlsext_host_name(stream.native_handle(), host)) {
        throw boost::system::system_error{boost::asio::error::invalid_argument};
    }
}

boost::asio::awaitable<void> SpotifyClient::fetchTokens(std::string code){
    using namespace boost::asio;
    using namespace boost::beast;
    try{
        // Make POST body
        std::ostringstream oss;
        oss << "grant_type=authorization_code"
            << "&code=" << encodeURL(code)
            << "&redirect_uri=" << encodeURL(redirectUri_)
            << "&client_id=" << encodeURL(clientId_)
            << "&code_verifier=" << encodeURL(codeVerifier_);

        const auto body = oss.str();
        auto& io_ctx = GlobalIoService::instance();

        // Resolving
        auto endpoints = co_await resolver_
            .async_resolve("accounts.spotify.com","443", use_awaitable);

        // SSL-stream
        ssl_stream<tcp_stream> stream{io_ctx, ssl_ctx_};
        configure_stream(stream, "accounts.spotify.com");


        // TCP connetcion
        co_await get_lowest_layer(stream)
            .async_connect(endpoints, use_awaitable);

        // Handshake
        co_await stream
            .async_handshake(ssl::stream_base::client, use_awaitable);

        // Form POST-request
        http::request<http::string_body> req{http::verb::post, "/api/token", 11};
        req.set(http::field::host, "accounts.spotify.com");
        req.set(http::field::user_agent, "ExportLikes/1.0");
        req.set(http::field::content_type, "application/x-www-form-urlencoded");
        req.set(http::field::content_length, std::to_string(body.size()));
        req.body() = body;
        req.prepare_payload();

        // Send request
        co_await async_write(stream, req, use_awaitable);

        // Prepare buffer and response
        flat_buffer buf;
        http::response<http::string_body> res;

        // Get response
        co_await http::async_read(stream, buf, res, use_awaitable);

        // Close session
        error_code ec;
        co_await stream.async_shutdown(redirect_error(use_awaitable, ec));
        if(ec == boost::asio::error::eof ||
            ec == boost::asio::ssl::error::stream_truncated){
            ec.clear();
        }
        if(ec){
            throw system_error{ec};
        }

        // Parse JSON-response
        QJsonDocument doc = QJsonDocument::fromJson(QByteArray::fromStdString(res.body()));
        if(!doc.isObject()){
            qDebug() << "Invalid token response\n";
            co_return;
        }

        QJsonObject obj = doc.object();
        accessToken_ = obj["access_token"].toString().toStdString();
        refreshToken_ = obj["refresh_token"].toString().toStdString();
        int expiresIn = obj["expires_in"].toInt();
        tokenExpiry_ = std::chrono::steady_clock::now() + std::chrono::seconds(expiresIn);

        qDebug() << "Access token received.";
        //qDebug() << "tokenExpiry_ = " << expiresIn;

    }
    catch(std::exception& e){
        qDebug() << "Error in fetchTokens: " << e.what() << "\n";
    }

    co_return;
}

boost::asio::awaitable<std::string> SpotifyClient::searchTrack(
    const std::string& artist, const std::string& title){
    using namespace boost::asio;
    using namespace boost::beast;
    try{
        // Make GET path
        std::string s;
        if(artist.empty()){
            s = "track:" + title;
        }
        else{
            s = "artist:" + artist + " track:" + title;
        }
        std::string path = "/v1/search?q=" + encodeURL(s)
                           + "&type=track&limit=1";


        // Resolve
        auto& io_ctx = GlobalIoService::instance();

        auto endpoints = co_await resolver_
            .async_resolve("api.spotify.com", "443", use_awaitable);

        // SSL-stream
        ssl_stream<tcp_stream> stream{io_ctx, ssl_ctx_};
        configure_stream(stream, "accounts.spotify.com");


        // TCP connection
        co_await get_lowest_layer(stream)
            .async_connect(endpoints, use_awaitable);

        // Handshake
        co_await stream
            .async_handshake(ssl::stream_base::client, use_awaitable);

        // Form GET-request
        http::request<http::string_body> req{http::verb::get, path, 11};
        req.set(http::field::host, "api.spotify.com");
        req.set(http::field::user_agent, "ExportLikes/1.0");
        req.set(http::field::authorization,
                std::string("Bearer ") + accessToken_);

        // Send request
        co_await http::async_write(stream, req, use_awaitable);

        // Prepare buffer and response
        flat_buffer buf;
        http::response<http::string_body> res;

        // Get response
        co_await http::async_read(stream, buf, res, use_awaitable);


        // Close TCP-sesson
        error_code ec;
        co_await stream.async_shutdown(redirect_error(use_awaitable, ec));
        if(ec == boost::asio::error::eof ||
            ec == boost::asio::ssl::error::stream_truncated){
            ec.clear();
        }
        if(ec){
            throw system_error{ec};
            co_return "";
        }

        //qDebug() << "searchTrack response status:" << res.result_int();
        //qDebug() << "response body:" << QString::fromStdString(res.body());

        //Check OK status
        if(res.result_int() != 200 && res.result_int() != 201){
            qDebug() << "Response status is not 200/201\n";
            co_return "";
        }

        // Parse Json-response
        QJsonDocument doc = QJsonDocument::fromJson(
            QByteArray::fromStdString(res.body()));

        if(!doc.isObject()){
            qDebug() << "doc from Json is not object\n";
            co_return "";
        }

        QJsonObject obj = doc.object();
        QJsonObject tracks = obj.value("tracks").toObject();
        QJsonArray items = tracks.value("items").toArray();
        if(items.isEmpty()){
            qDebug() << "items from Json is empty\n";
            co_return "";
        }
        QJsonObject first = items[0].toObject();
        QString id = first.value("id").toString();

        co_return id.toStdString();
    }
    catch(std::exception& e){
        qDebug() << "Error in SearchTrack(" << artist
                 << ", " << title << "): " << e.what() << "\n";
        co_return "";
    }
}

boost::asio::awaitable<void> SpotifyClient::likeTracksFromJson(const std::string& jsonPath,
        std::function<void(int, int)> progressCb){
    try{
        // Read json-file with tracks
        auto path = QString::fromStdString(jsonPath);
        QFile file{path};
        if(!file.open(QIODevice::ReadOnly)){
            qDebug() << "Error in opening file\n";
            co_return;
        }
        QByteArray data = file.readAll();
        file.close();


        // Prepare for parsing
        QJsonParseError er;
        QJsonDocument doc = QJsonDocument::fromJson(data, &er);
        if(er.error != QJsonParseError::NoError || !doc.isArray()){
            qWarning() << "Invalid JSON file:" << er.errorString();
            co_return;
        }

        QJsonArray arr = doc.array();
        std::vector<std::string> trackIds;

        int total = arr.size();
        int count = 0;

        // Parsing traсks from json and get their ids
        for(const auto& val : arr){
            if(!val.isObject()){
                continue;
            }
            QJsonObject obj = val.toObject();
            QString artist = obj.value("artist").toString();
            QString title = obj.value("title").toString();

            if(title.isEmpty()){
                continue;
            }

            auto id = co_await searchTrack(artist.toStdString(), title.toStdString());

            // Callback to get progress
            ++count;
            if (progressCb) progressCb(count, total);

            if (id.empty()){
                continue;
            }
            trackIds.push_back(id);
            if(trackIds.size() == 50){
                co_await addTracksToLibrary(trackIds);
                trackIds.clear();
            }
        }

        // Send to add-function remained ids
        if(!trackIds.empty()){
            co_await addTracksToLibrary(trackIds);
            trackIds.clear();
        }

        qDebug() << "✅ All tracks processed and liked";
    }
    catch(std::exception& e){
        qWarning() << "Error in likeTracksFromJson: " << e.what();
    }
}

boost::asio::awaitable<void> SpotifyClient::addTracksToLibrary(
    const std::vector<std::string>& trackIds){
    using namespace boost::asio;
    using namespace boost::beast;
    try{
        // Make tracks string for PUT-request
        std::string tracksString;
        for(size_t i = 0; i < trackIds.size(); i++){
            tracksString += trackIds[i];
            if(i < trackIds.size() - 1){
                tracksString += ",";
            }
        }

        // Resolving
        auto& io_ctx = GlobalIoService::instance();
        auto endpoints = co_await resolver_
            .async_resolve("api.spotify.com", "443", use_awaitable);

        // SSL-stream
        ssl_stream<tcp_stream> stream{io_ctx, ssl_ctx_};
        configure_stream(stream, "accounts.spotify.com");

        // TCP connection
        co_await get_lowest_layer(stream)
            .async_connect(endpoints, use_awaitable);

        // Handshake
        co_await stream
            .async_handshake(ssl::stream_base::client, use_awaitable);

        // Form PUT-request
        std::string target = "/v1/me/tracks?ids=" + encodeURL(tracksString);
        http::request<http::empty_body> req{http::verb::put, target, 11};
        req.set(http::field::host, "api.spotify.com");
        req.set(http::field::user_agent, "Export:ikes 1.0");
        req.set(http::field::authorization, "Bearer " + accessToken_);
        req.prepare_payload();

        // Send POST-request
        co_await http::async_write(stream, req, use_awaitable);

        // Prepare buffer and response
        flat_buffer buf;
        http::response<http::string_body> res;

        // Get response
        co_await http::async_read(stream, buf, res, use_awaitable);

        // Close session
        error_code ec;
        co_await stream.async_shutdown(redirect_error(use_awaitable, ec));
        if(ec == boost::asio::error::eof ||
            ec == boost::asio::ssl::error::stream_truncated){
            ec.clear();
        }
        if(ec){
            throw system_error{ec};
        }
        //qWarning() << "addTracks response status:" << res.result_int();
        //qWarning() << "addTracks response body:" << QString::fromStdString(res.body());

        // Check OK status
        int code = res.result_int();
        if (code != 200 && code != 201) {
            qWarning() << "Failed to save" << trackIds.size()
            << "tracks, HTTP status:" << code;
        } else {
            qDebug() << "Saved" << trackIds.size() << "tracks successfully.";
        }
    }
    catch(std::exception& e){
        qWarning() << "Error in addTracksToLibrary: " << e.what();

    }

    co_return;
}

boost::asio::awaitable<void> SpotifyClient::removeLastN(std::size_t n,
    std::function<void(int, int)> progressCb){
    using namespace boost::asio;
    using namespace boost::beast;

    int total = n;
    int count = 0;

    try{

        while(n > 0){
            // Split into bacthes
            auto batch = std::min<std::size_t>(n, 50);

            // Resolving
            auto& io_ctx = GlobalIoService::instance();
            auto endpoints = co_await resolver_
                                 .async_resolve("api.spotify.com", "443", use_awaitable);

            // SSL-stream
            ssl_stream<tcp_stream> stream{io_ctx, ssl_ctx_};
            configure_stream(stream, "api.spotify.com");

            // TCP connection
            co_await get_lowest_layer(stream)
                .async_connect(endpoints, use_awaitable);

            // Handshake
            co_await stream
                .async_handshake(ssl::stream_base::client, use_awaitable);

            // Form GET-request
            std::string target = "/v1/me/tracks?limit=" + std::to_string(batch);
            http::request<http::empty_body> req{http::verb::get, target, 11};
            req.set(http::field::host, "api.spotify.com");
            req.set(http::field::user_agent, "ExportLikes 1.0");
            req.set(http::field::authorization, "Bearer " + accessToken_);

            // Send request
            co_await http::async_write(stream, req, use_awaitable);

            // Prepare buffer and response
            flat_buffer buf;
            http::response<http::string_body> res;

            // Get response
            co_await http::async_read(stream, buf, res, use_awaitable);

            // Close session
            boost::system::error_code ec;
            co_await stream.async_shutdown(redirect_error(use_awaitable, ec));
            if(ec == boost::asio::error::eof ||
                ec == boost::asio::ssl::error::stream_truncated){
                ec.clear();
            }
            if(ec){
                throw system_error{ec};
            }

            if(res.result_int() != 200 && res.result_int() != 201){
                qWarning() << "Get /me/tracks failed: " << res.result_int()
                           << "\n" << res.body();
                co_return;
            }

            // Parse the response
            QJsonDocument doc = QJsonDocument::fromJson(
                QByteArray::fromStdString(res.body()));

            if(!doc.isObject()){
                qDebug() << "doc from Json is not object\n";
                co_return;
            }

            QJsonObject obj = doc.object();
            QJsonArray items = obj.value("items").toArray();

            if(items.isEmpty()){
                qDebug() << "No tracks to remove";
                co_return;
            }
            std::vector<std::string> ids;
            for (auto it = items.begin(); it != items.end(); it++){
                auto trackObj = it->toObject()
                .value("track")
                    .toObject();
                ids.push_back(trackObj.value("id").toString().toStdString());
            }

            co_await sendRemoveReq(ids);

            count += batch;
            if (progressCb) progressCb(count, total);

            n -= batch;
        }

    }
    catch(std::exception& e){
        qWarning() << "Error in removeLastN: " << e.what();
    }
    co_return;
}

boost::asio::awaitable<void> SpotifyClient::sendRemoveReq(const std::vector<std::string>& ids){
    using namespace boost::asio;
    using namespace boost::beast;

    try{
        // Resolving
        auto& io_ctx = GlobalIoService::instance();
        auto endpoints = co_await resolver_
            .async_resolve("api.spotify.com", "443", use_awaitable);

        // SSL-stream
        ssl_stream<tcp_stream> stream{io_ctx, ssl_ctx_};
        configure_stream(stream, "api.spotify.com");

        // TCP connection
        co_await get_lowest_layer(stream).async_connect(endpoints, use_awaitable);

        // Handshake
        co_await stream.async_handshake(ssl::stream_base::client, use_awaitable);

        // Form DELETE-request
        std::string idsString;
        for (std::size_t i = 0; i < ids.size(); i++){
            idsString += ids[i];
            if(i + 1 < ids.size()){
                idsString += ",";
            }
        }
        std::string target = "/v1/me/tracks?ids=" + idsString;
        http::request<http::empty_body> req{http::verb::delete_, target, 11};
        req.set(http::field::host, "api.spotify.com");
        req.set(http::field::user_agent, "ExportLikes 1.0");
        req.set(http::field::authorization, "Bearer " + accessToken_);

        // Send request
        co_await http::async_write(stream, req, use_awaitable);

        // Prepare buffer and response
        flat_buffer buf;
        http::response<http::string_body> res;

        // Get response
        co_await http::async_read(stream, buf, res, use_awaitable);

        // Close session
        error_code ec;
        co_await stream.async_shutdown(redirect_error(use_awaitable, ec));
        if(ec == boost::asio::error::eof ||
            ec == boost::asio::ssl::error::stream_truncated){
            ec.clear();
        }
        if(ec){
            throw system_error{ec};
        }

        if(res.result_int() != 200 && res.result_int() != 201 &&
            res.result_int() != 204){
            qWarning() << "DELETE failed: " << res.result_int();
            co_return;
        }
    }
    catch(std::exception& e){
        qWarning() << "Error in sendRemoveReq: " << e.what();
    }
    co_return;
}

bool SpotifyClient::hasValidAccessToken() const{
    return !accessToken_.empty()
    && std::chrono::steady_clock::now() < tokenExpiry_;
}
