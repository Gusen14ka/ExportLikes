#pragma once

#include<string>

// Function to generate code verifier (length is 64) for PKCE
// Using mt19937
std::string generateCodeVerifier();

// Encode to base64
std::string encodeBase64(const std::string& val);

// Overload for const unsigned char*
std::string encodeBase64(const unsigned char* val, std::size_t length);

// Encode from base64 to base64URL
// Change '/' -> '_', '+' -> '-' and delete '='
std::string encodeBase64Url(std::string& base64);

// Generate code-challange for PKCE from code-verifier
// Using SHA256 hash and base64URL
std::string generateCodeChallange(std::string verifier);
