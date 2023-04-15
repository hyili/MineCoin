#include "utils.hpp"
#include <boost/json/src.hpp>

void load_keys(std::string &access_key, std::string &secret_key) {
  std::ifstream ifs;
  ifs.open("conf/access", std::ifstream::in);
  ifs >> access_key;
  ifs.close();
  ifs.open("conf/secret", std::ifstream::in);
  ifs >> secret_key;
  ifs.close();
}

std::string sha256(const std::string &raw) {
  uint8_t result[SHA256_DIGEST_LENGTH];

  SHA256_CTX sha256;
  SHA256_Init(&sha256);
  SHA256_Update(&sha256, raw.c_str(), raw.size());
  SHA256_Final(result, &sha256);

  std::stringstream ss;
  for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
    ss << std::hex << std::setw(2) << std::setfill('0')
       << static_cast<int>(result[i]);
  }

  return ss.str();
}

std::string base64_encode(const std::string &raw) {
  auto result_size = base64::encoded_size(raw.size());
  auto *ptr = malloc(sizeof(char) * (result_size + 1));
  ((char *)ptr)[result_size] = '\0';

  base64::encode(ptr, raw.c_str(), raw.size());
  std::string result = std::string((char *)ptr);

  free(ptr);

  return result;
}

std::string hmac(std::string &algo, const std::string &secret,
                 const std::string &raw) {
  const EVP_MD *engine = nullptr;
  if (algo == "sha256") {
    engine = EVP_sha256();
  }

  if (engine) {
    uint8_t hmac_result[EVP_MAX_MD_SIZE];
    HMAC_CTX *hmac = HMAC_CTX_new();
    HMAC_Init_ex(hmac, secret.c_str(), secret.size(), engine, nullptr);
    HMAC_Update(hmac, (const unsigned char *)raw.c_str(), raw.size());
    uint32_t len = EVP_MAX_MD_SIZE;
    HMAC_Final(hmac, hmac_result, &len);

    std::stringstream ss;
    for (int i = 0; i < len; i++) {
      ss << std::hex << std::setw(2) << std::setfill('0')
         << static_cast<int>(hmac_result[i]);
    }

    return ss.str();
  }

  return "";
}
