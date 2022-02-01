/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2022-02-01
    License: MIT

    Simple refactoring of the static_asserts for the utilities.
    Because the silly compiler would test in every translation module.
*/

#include <Stdinclude.hpp>

namespace Testing
{
    // Crypto/Hashes.hpp
    static_assert(Hash::WW32("12345") == 0xEE98FD70, "Someone fucked with WW32.");
    static_assert(Hash::FNV1_32("12345") == 0xDEEE36FA, "Someone fucked with FNV32.");
    static_assert(Hash::CRC32A("12345") == 0x426548B8, "Someone fucked with CRC32-A.");
    static_assert(Hash::CRC32B("12345") == 0xCBF53A1C, "Someone fucked with CRC32-B.");
    static_assert(Hash::CRC32T("12345") == 0x0315B56C, "Someone fucked with CRC32-T.");
    static_assert(Hash::FNV1a_32("12345") == 0x43C2C0D8, "Someone fucked with FNV32a.");
    static_assert(Hash::WW64("12345") == 0x3C570C468027DB01ULL, "Someone fucked with WW64.");
    static_assert(Hash::FNV1_64("12345") == 0xA92F4455DA95A77AULL, "Someone fucked with FNV64.");
    static_assert(Hash::FNV1a_64("12345") == 0xE575E8883C0F89F8ULL, "Someone fucked with FNV64a.");

    // Encoding/Base64.hpp
    static_assert(Base64::Decode("MTIzNDU=") == Base64::Decode(Base64::Encode("12345")), "Someone fucked with the Base64 encoding..");

    // Encoding/Base85.hpp
    static_assert(Base85::Z85::Decode(Base85::Z85::Encode("1234")) == Base85::RFC1924::Decode(Base85::RFC1924::Encode("1234")), "Someone fucked with the Base85 encoding..");
}
