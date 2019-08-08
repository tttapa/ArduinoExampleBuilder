#include <openssl/md5.h>
#include <vector>

#include <StringHelpers.hpp>

static inline char nibbletohex(uint8_t n) {
    n &= 0x0F;
    return n + ((n < 10) ? '0' : 'a' - 10);
}

std::string md5(const std::string &s) {
    std::vector<uint8_t> result;
    result.resize(128 / 8);
    MD5((const uint8_t *)s.data(), s.size(), result.data());
    std::string resultstr;
    resultstr.resize(2 * 128 / 8);
    for (size_t i = 0; i < result.size(); ++i) {
        resultstr[2 * i] = nibbletohex(result[i] >> 4);
        resultstr[2 * i + 1] = nibbletohex(result[i] >> 0);
    }
    return resultstr;
}