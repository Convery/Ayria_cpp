/*
Initial author: Convery (tcn@ayria.se)
Started: 2020-02-10
License: MIT
*/

#pragma once
#if defined (HAS_OPENSSL)
#include <Stdinclude.hpp>
#include <openssl/ecdsa.h>
#include <openssl/rand.h>
#include <openssl/x509.h>
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <openssl/rsa.h>
#include <openssl/ssl.h>

namespace AES
{
    inline std::string Encrypt(const void *Input, const size_t Length, const void *Key, const void *Initialvector)
    {
        const auto Buffer = std::make_unique<uint8_t[]>(Length + 32);
        const auto Context = EVP_CIPHER_CTX_new();
        int Encryptionlength = 0;

        EVP_EncryptInit_ex(Context, EVP_aes_128_cbc(), nullptr, (unsigned char *)Key, (unsigned char *)Initialvector);
        EVP_EncryptUpdate(Context, Buffer.get(), &Encryptionlength, (unsigned char *)Input, uint32_t(Length));
        EVP_EncryptFinal_ex(Context, Buffer.get() + Encryptionlength, &Encryptionlength);
        EVP_CIPHER_CTX_free(Context);

        return std::string((char *)Buffer.get(), Length);
    }
    inline std::string Decrypt(const void *Input, const size_t Length, const void *Key, const void *Initialvector)
    {
        const auto Buffer = std::make_unique<uint8_t[]>(Length + 32);
        const auto Context = EVP_CIPHER_CTX_new();
        int Decryptionlength = 0;

        EVP_DecryptInit_ex(Context, EVP_aes_128_cbc(), nullptr, (unsigned char *)Key, (unsigned char *)Initialvector);
        EVP_DecryptUpdate(Context, Buffer.get(), &Decryptionlength, (unsigned char *)Input, uint32_t(Length));
        EVP_DecryptFinal_ex(Context, Buffer.get() + Decryptionlength, &Decryptionlength);
        EVP_CIPHER_CTX_free(Context);

        return std::string((char *)Buffer.get(), Length);
    }
}

namespace DES3
{
    inline std::string Encrypt(const void *Input, const size_t Length, const void *Key, const void *Initialvector)
    {
        const auto Buffer = std::make_unique<uint8_t[]>(Length + 32);
        const auto Context = EVP_CIPHER_CTX_new();
        int Encryptionlength = 0;

        EVP_EncryptInit_ex(Context, EVP_des_ede3_cbc(), nullptr, (unsigned char *)Key, (unsigned char *)Initialvector);
        EVP_EncryptUpdate(Context, Buffer.get(), &Encryptionlength, (unsigned char *)Input, uint32_t(Length));
        EVP_EncryptFinal_ex(Context, Buffer.get() + Encryptionlength, &Encryptionlength);
        EVP_CIPHER_CTX_free(Context);

        return std::string((char *)Buffer.get(), Length);
    }
    inline std::string Decrypt(const void *Input, const size_t Length, const void *Key, const void *Initialvector)
    {
        const auto Buffer = std::make_unique<uint8_t[]>(Length + 32);
        const auto Context = EVP_CIPHER_CTX_new();
        int Decryptionlength = 0;

        EVP_DecryptInit_ex(Context, EVP_des_ede3_cbc(), nullptr, (unsigned char *)Key, (unsigned char *)Initialvector);
        EVP_DecryptUpdate(Context, Buffer.get(), &Decryptionlength, (unsigned char *)Input, uint32_t(Length));
        EVP_DecryptFinal_ex(Context, Buffer.get() + Decryptionlength, &Decryptionlength);
        EVP_CIPHER_CTX_free(Context);

        return std::string((char *)Buffer.get(), Length);
    }
}

namespace Hash
{
    inline std::string SHA1(const void *Input, const size_t Size)
    {
        const auto Buffer = std::make_unique<uint8_t[]>(20);
        const auto Context = EVP_MD_CTX_create();

        EVP_DigestInit_ex(Context, EVP_sha1(), nullptr);
        EVP_DigestUpdate(Context, Input, Size);
        EVP_DigestFinal_ex(Context, Buffer.get(), nullptr);
        EVP_MD_CTX_destroy(Context);

        return std::string((char *)Buffer.get(), 20);
    }
    inline std::string SHA256(const void *Input, const size_t Size)
    {
        const auto Buffer = std::make_unique<uint8_t[]>(32);
        const auto Context = EVP_MD_CTX_create();

        EVP_DigestInit_ex(Context, EVP_sha256(), nullptr);
        EVP_DigestUpdate(Context, Input, Size);
        EVP_DigestFinal_ex(Context, Buffer.get(), nullptr);
        EVP_MD_CTX_destroy(Context);

        return std::string((char *)Buffer.get(), 32);
    }
    inline std::string HMACSHA1(const void *Input, const size_t Size, const void *Key, const size_t Keysize)
    {
        unsigned char Buffer[256]{};
        unsigned int Buffersize = 256;

        HMAC(EVP_sha1(), Key, uint32_t(Keysize), (uint8_t *)Input, Size, Buffer, &Buffersize);
        return std::string((char *)Buffer, Buffersize);
    }
    inline std::string HMACSHA256(const void *Input, const size_t Size, const void *Key, const size_t Keysize)
    {
        unsigned char Buffer[256]{};
        unsigned int Buffersize = 256;

        HMAC(EVP_sha256(), Key, uint32_t(Keysize), (uint8_t *)Input, Size, Buffer, &Buffersize);
        return std::string((char *)Buffer, Buffersize);
    }
    inline std::string MD5(const void *Input, const size_t Size)
    {
        const auto Buffer = std::make_unique<uint8_t[]>(16);
        const auto Context = EVP_MD_CTX_create();

        EVP_DigestInit_ex(Context, EVP_md5(), nullptr);
        EVP_DigestUpdate(Context, Input, Size);
        EVP_DigestFinal_ex(Context, Buffer.get(), nullptr);
        EVP_MD_CTX_destroy(Context);

        return std::string((char *)Buffer.get(), 16);
    }
}

namespace PK_RSA
{
    inline RSA *Createkeypair(int Size = 2048)
    {
        const auto Key = RSA_new();
        const auto Exponent = BN_new();
        do
        {
            if (1 != BN_set_word(Exponent, 65537)) break;
            if (1 != RSA_generate_key_ex(Key, Size, Exponent, nullptr)) break;
        } while (false);

        BN_free(Exponent);
        return Key;
    }
    template <typename T> inline RSA *Createkeypair(std::basic_string_view<T> Privatekey)
    {
        const auto Keypointer = Privatekey.data();
        return d2i_RSAPrivateKey(NULL, (const uint8_t **)&Keypointer, (long)Privatekey.size());
    }
    template <typename T> inline RSA *Createkeypair(const std::basic_string<T> &Privatekey)
    {
        return Createkeypair(std::basic_string_view<T>(Privatekey.data(), Privatekey.size()));
    }

    template <typename T> inline std::string Signmessage(std::basic_string_view<T> Input, RSA *Key)
    {
        EVP_MD_CTX *Context = EVP_MD_CTX_create();
        EVP_PKEY *Privatekey = EVP_PKEY_new();
        EVP_PKEY_assign_RSA(Privatekey, Key);

        EVP_PKEY_CTX *pkeyCtx;
        EVP_DigestSignInit(Context, &pkeyCtx, EVP_sha256(), nullptr, Privatekey);
        EVP_PKEY_CTX_set_rsa_padding(pkeyCtx, RSA_PKCS1_PSS_PADDING);
        EVP_PKEY_CTX_set_rsa_pss_saltlen(pkeyCtx, 0);
        EVP_DigestSignUpdate(Context, Input.data(), Input.size());

        size_t Signaturelength{};
        EVP_DigestSignFinal(Context, nullptr, &Signaturelength);
        std::string Signature{}; Signature.resize(Signaturelength);
        EVP_DigestSignFinal(Context, (unsigned char *)Signature.data(), &Signaturelength);
        EVP_MD_CTX_destroy(Context);

        return Signature;
    }
    template <typename T> inline std::string Signmessage(const std::basic_string<T> &Input, RSA *Key)
    {
        return Signmessage(std::basic_string_view<T>(Input.data(), Input.size()), Key);
    }

    template <typename T> inline bool Verifysignature(std::basic_string_view<T> Input, std::basic_string_view<T> Signature, RSA *Key)
    {
        EVP_MD_CTX *Context = EVP_MD_CTX_create();
        EVP_PKEY *Publickey = EVP_PKEY_new();
        EVP_PKEY_assign_RSA(Publickey, Key);

        EVP_PKEY_CTX *pkeyCtx;
        EVP_DigestSignInit(Context, &pkeyCtx, EVP_sha256(), nullptr, Publickey);
        EVP_PKEY_CTX_set_rsa_padding(pkeyCtx, RSA_PKCS1_PSS_PADDING);
        EVP_PKEY_CTX_set_rsa_pss_saltlen(pkeyCtx, 0);

        EVP_DigestVerifyInit(Context, &pkeyCtx, EVP_sha256(), nullptr, Publickey);
        EVP_DigestVerifyUpdate(Context, Input.data(), Input.size());

        const auto Result = 1 == EVP_DigestVerifyFinal(Context, (uint8_t *)Signature.data(), Signature.size());

        EVP_MD_CTX_destroy(Context);
        return Result;
    }
    template <typename T> inline bool Verifysignature(const std::basic_string<T> &Input, const std::basic_string<T> &Signature, RSA *Key)
    {
        return Verifysignature(std::basic_string_view<T>(Input.data(), Input.size()), std::basic_string_view<T>(Signature.data(), Signature.size()), Key);
    }
    template <typename T> inline bool Verifysignature(const std::basic_string<T> &Input, const std::basic_string<T> &Signature, std::string_view Publickey)
    {
        const auto Keypointer = Publickey.data();
        const auto Key = d2i_RSA_PUBKEY(NULL, (const uint8_t **)&Keypointer, (long)Publickey.size());
        if (!Key) return false;

        return Verifysignature(std::basic_string_view<T>(Input.data(), Input.size()), std::basic_string_view<T>(Signature.data(), Signature.size()), Key);
    }

    template <typename T> inline std::string Encrypt(std::basic_string_view<T> Input, RSA *Key)
    {
        std::string Result; Result.resize(Input.size());
        if (Input.size() != RSA_public_encrypt((int)Input.size(), (const uint8_t *)Input.data(),
                                               (uint8_t *)Result.data(), Key, RSA_NO_PADDING))
            return "";

        return Result;
    }
    template <typename T> inline std::string Decrypt(std::basic_string_view<T> Input, RSA *Key)
    {
        std::string Result; Result.resize(Input.size());
        if (Input.size() != RSA_private_decrypt((int)Input.size(), (const uint8_t *)Input.data(),
                                                (uint8_t *)Result.data(), Key, RSA_NO_PADDING))
            return "";

        return Result;
    }
    template <typename T> inline std::string Encrypt(std::basic_string_view<T> Input, std::string_view Publickey)
    {
        const auto Keypointer = Publickey.data();
        const auto Key = d2i_RSA_PUBKEY(NULL, (const uint8_t **)&Keypointer, (long)Publickey.size());
        if (!Key) return "";    // WTF?

        return Encrypt(Input, Key);
    }
    template <typename T> inline std::string Decrypt(std::basic_string_view<T> Input, std::string_view Privatekey)
    {
        const auto Keypointer = Privatekey.data();
        const auto Key = d2i_RSAPrivateKey(NULL, (const uint8_t **)&Keypointer, (long)Privatekey.size());
        if (!Key) return "";    // WTF?

        return Decrypt(Input, Key);
    }

    template <typename T> inline std::string Encrypt(const std::basic_string<T> &Input, RSA *Key)
    {
        return Encrypt(std::basic_string_view<T>(Input.data(), Input.size()), Key);
    }
    template <typename T> inline std::string Decrypt(const std::basic_string<T> &Input, RSA *Key)
    {
        return Decrypt(std::basic_string_view<T>(Input.data(), Input.size()), Key);
    }
    template <typename T> inline std::string Encrypt(const std::basic_string<T> &Input, std::string_view Publickey)
    {
        return Encrypt(std::basic_string_view<T>(Input.data(), Input.size()), Publickey);
    }
    template <typename T> inline std::string Decrypt(const std::basic_string<T> &Input, std::string_view Privatekey)
    {
        return Decrypt(std::basic_string_view<T>(Input.data(), Input.size()), Privatekey);
    }

    inline std::string getPublickey(RSA *Key)
    {
        const auto Bio = BIO_new(BIO_s_mem());
        i2d_RSA_PUBKEY_bio(Bio, Key);

        const auto Length = BIO_pending(Bio);
        std::string Result; Result.resize(Length);
        BIO_read(Bio, Result.data(), Length);

        BIO_free_all(Bio);
        return Result;
    }
    inline std::string getPrivatekey(RSA *Key)
    {
        const auto Bio = BIO_new(BIO_s_mem());
        i2d_RSAPrivateKey_bio(Bio, Key);

        const auto Length = BIO_pending(Bio);
        std::string Result; Result.resize(Length);
        BIO_read(Bio, Result.data(), Length);

        BIO_free_all(Bio);
        return Result;
    }

    // We send data without padding, so add your own for security if needed.
    inline std::string RSAPadding(std::string_view Input, std::string_view Padding, size_t Finallength)
    {
        std::string Result; Result.resize(Finallength);

        if (1 != RSA_padding_add_PKCS1_OAEP((uint8_t *)Result.data(), (int)Finallength,
                                            (const uint8_t *)Input.data(), (int)Input.size(),
                                            (const uint8_t *)Padding.data(), (int)Padding.size()))
            return "";

        return Result;
    }

    inline std::string fromLibtom(std::string_view Key)
    {
        std::string Opensslkey;
        {
            const std::string Algorithmpart((char *)"\x30\x0D\x06\x09\x2A\x86\x48\x86\xF7\x0D\x01\x01\x01\x05\x00", 15);
            std::string Publickeypart((char *)"\x03\x81\x8D\x00", 4);
            Publickeypart.append(Key);

            Opensslkey.push_back('\x30');
            Opensslkey.push_back('\x81');
            Opensslkey.push_back(uint8_t(Algorithmpart.size() + Publickeypart.size()));
            Opensslkey.append(Algorithmpart);
            Opensslkey.append(Publickeypart);
        }
        return Opensslkey;
    }
    inline std::string toLibtom(std::string_view Key)
    {
        Key.remove_prefix(15 + 4 + 3);
        return std::string(Key.data(), Key.size());
    }
}

namespace PK_ECC
{
    inline EC_KEY *Createkeypair(int Curve = NID_secp521r1)
    {
        const auto Key = EC_KEY_new_by_curve_name(Curve);
        if (1 != EC_KEY_generate_key(Key)) return nullptr;
        if (1 != EC_KEY_check_key(Key)) return nullptr;
        return Key;
    }
    inline std::string Signmessage(std::string_view Input, EC_KEY *Key)
    {
        EVP_MD_CTX *Context = EVP_MD_CTX_create();
        EVP_PKEY *Privatekey = EVP_PKEY_new();
        EVP_PKEY_assign_EC_KEY(Privatekey, Key);

        EVP_DigestSignInit(Context, nullptr, EVP_sha256(), nullptr, Privatekey);
        EVP_DigestSignUpdate(Context, Input.data(), Input.size());

        size_t Signaturelength{};
        EVP_DigestSignFinal(Context, nullptr, &Signaturelength);
        std::string Signature{}; Signature.resize(Signaturelength);
        EVP_DigestSignFinal(Context, (uint8_t *)Signature.data(), &Signaturelength);
        EVP_MD_CTX_destroy(Context);

        return Signature;
    }
    inline bool Verifysignature(std::string_view Input, std::string_view Signature, EC_KEY *Key)
    {
        EVP_MD_CTX *Context = EVP_MD_CTX_create();
        EVP_PKEY *Publickey = EVP_PKEY_new();
        EVP_PKEY_assign_EC_KEY(Publickey, Key);

        EVP_DigestVerifyInit(Context, nullptr, EVP_sha256(), nullptr, Publickey);
        EVP_DigestVerifyUpdate(Context, Input.data(), Input.size());

        const auto Result = 1 == EVP_DigestVerifyFinal(Context, (uint8_t *)Signature.data(), Signature.size());

        EVP_MD_CTX_destroy(Context);
        return Result;
    }

    inline std::string getPublickey(EC_KEY *Key)
    {
        const auto Bio = BIO_new(BIO_s_mem());
        i2d_EC_PUBKEY_bio(Bio, Key);

        const auto Length = BIO_pending(Bio);
        std::string Result; Result.resize(Length);
        BIO_read(Bio, Result.data(), Length);

        BIO_free_all(Bio);
        return Result;
    }
}

#endif
