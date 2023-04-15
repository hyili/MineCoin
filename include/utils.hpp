
#ifndef BOOST_UTILS_HPP
#define BOOST_UTILS_HPP

#include <string>
#include <fstream>
#include <openssl/sha.h>
#include <openssl/hmac.h>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <boost/beast.hpp>
#include <boost/json.hpp>

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace base64 = beast::detail::base64;
namespace json = boost::json;

void load_keys(std::string &access_key, std::string &secret_key);
std::string sha256(const std::string &raw);
std::string base64_encode(const std::string &raw);
std::string hmac(std::string& algo, const std::string& secret, const std::string& raw);

#endif
