#include <random>
#include <string>
#include <openssl/sha.h>
#include <boost/beast/core/detail/base64.hpp>
#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/transform_width.hpp>
#include <boost/algorithm/string.hpp>
#include "HelperPKCE.hpp"

// Function to generate code verifier (length is 64) for PKCE
// Using mt19937
std::string generateCodeVerifier(){
    static constexpr char charset[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789-._~";

    std::mt19937 gen{std::random_device{}()};
    std::uniform_int_distribution<std::size_t> dist(0, sizeof(charset) - 2);

    std::string verifier;
    verifier.reserve(64);
    for(std::size_t i = 0; i < 64; i++){
        verifier += charset[dist(gen)];
    }
    return verifier;
}

// Encode to base64
std::string encodeBase64(const std::string& val){
    using namespace boost::archive::iterators;
    using It = base64_from_binary<transform_width<std::string::const_iterator, 6, 8>>;
    auto tmp = std::string(It(std::begin(val)), It(std::end(val)));
    return tmp.append((3 - val.size() % 3) % 3, '=');
}

// Overload for const unsigned char*
std::string encodeBase64(const unsigned char* val, std::size_t length){
    using namespace boost::archive::iterators;
    using It = base64_from_binary<transform_width<const unsigned char*, 6, 8>>;
    auto tmp = std::string(It(val), It(val + length));
    return tmp.append((3 - length % 3) % 3, '=');
}

// Encode from base64 to base64URL
// Change '/' -> '_', '+' -> '-' and delete '='
std::string encodeBase64Url(std::string& base64){
    for(auto& c : base64){
        if(c == '+'){
            c = '-';
        }
        else if(c == '/'){
            c = '_';
        }
    }
    base64.erase((std::remove(base64.begin(), base64.end(), '=')), base64.end());
    return base64;
}

// Generate code-challange for PKCE from code-verifier
// Using SHA256 hash and base64URL
std::string generateCodeChallange(std::string verifier){
    // Make SHA256 hash from verifier
    unsigned char hash[SHA256_DIGEST_LENGTH];
    // OpenSSL's SHA256 function expects a pointer to unsigned bytes (const unsigned char*).
    // std::string::data() returns a pointer to const char, which is a distinct type in C++.
    // We use reinterpret_cast to convert const char* to const unsigned char*.
    //
    // This is safe because reading object representation bytes via unsigned char*
    // is allowed by the C++ standard (known as the "aliasing rule").
    // Effectively, we are treating the string's data as raw bytes without modifying it.
    SHA256(reinterpret_cast<const unsigned char*>(verifier.data())
           ,verifier.size(), hash);

    // Encode to base64
    std::string base64 = encodeBase64(hash, SHA256_DIGEST_LENGTH);
    return encodeBase64Url(base64);
}
