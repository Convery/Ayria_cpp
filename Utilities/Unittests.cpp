/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2022-02-01
    License: MIT

    Simple refactoring of the static_asserts for the utilities.
    Because the silly compiler would test in every translation module.
*/

#include <Stdinclude.hpp>
#include "Crypto/SHA.hpp"

namespace Testing
{
    // Crypto/Hashes.hpp
    static_assert(Hash::WW32("12345") == 0xEE98FD70, "Someone fucked with WW32..");
    static_assert(Hash::FNV1_32("12345") == 0xDEEE36FA, "Someone fucked with FNV32..");
    static_assert(Hash::CRC32A("12345") == 0x426548B8, "Someone fucked with CRC32-A..");
    static_assert(Hash::CRC32B("12345") == 0xCBF53A1C, "Someone fucked with CRC32-B..");
    static_assert(Hash::CRC32T("12345") == 0x0315B56C, "Someone fucked with CRC32-T..");
    static_assert(Hash::FNV1a_32("12345") == 0x43C2C0D8, "Someone fucked with FNV32a..");
    static_assert(Hash::WW64("12345") == 0x3C570C468027DB01ULL, "Someone fucked with WW64..");
    static_assert(Hash::FNV1_64("12345") == 0xA92F4455DA95A77AULL, "Someone fucked with FNV64..");
    static_assert(Hash::FNV1a_64("12345") == 0xE575E8883C0F89F8ULL, "Someone fucked with FNV64a..");

    // Encoding/Base58.hpp
    static_assert(Base58::Decode("6YvUFcg") == Base58::Decode(Base58::Encode("12345")), "Someone fucked with the Base58 encoding..");

    // Encoding/Base64.hpp
    static_assert(Base64::Decode("MTIzNDU=") == Base64::Decode(Base64::Encode("12345")), "Someone fucked with the Base64 encoding..");

    // Encoding/Base85.hpp
    static_assert(Base85::Z85::Decode(Base85::Z85::Encode("1234")) == Base85::RFC1924::Decode(Base85::RFC1924::Encode("1234")), "Someone fucked with the Base85 encoding..");

    // Crypto/SHA.hpp
    constexpr auto SHATest = []() -> bool
    {
        // 5994471abb01112afcc18159f6cc74b4f511b99806da59b3caf5a9c173cacfc5
        constexpr auto Test256 = Hash::SHA256("12345");
        static_assert(Test256[0] == 0x59 && Test256[1] == 0x94 && Test256[2] == 0x47, "SHA256 is borked..");

        // 3627909a29c31381a071ec27f7c9ca97726182aed29a7ddd2e54353322cfb30abb9e3a6df2ac2c20fe23436311d678564d0c8d305930575f60e2d3d048184d79
        constexpr auto Test512 = Hash::SHA512("12345");
        static_assert(Test512[0] == 0x36 && Test512[1] == 0x27 && Test512[2] == 0x90, "SHA512 is borked..");

        return true;
    }();

}
