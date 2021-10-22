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
#include <string>


namespace AES
{
    template <typename T> concept Range_t = requires (const T && t) { t.data(); t.size(); sizeof(t[0]) == 1; };

    template <typename A = uint8_t, Range_t B, Range_t C, Range_t D> std::basic_string<A> Encrypt_128(B &&Cryptokey, C &&Initialvector, D &&Plaintext)
    {
        const auto Buffer = std::make_unique<uint8_t []>(Plaintext.size() + ((Plaintext.size() & 15) == 0 ? 0 : (16 - (Plaintext.size() & 15))));
        EVP_CIPHER_CTX *Context = EVP_CIPHER_CTX_new();
        int Encryptionlength = 0;

        assert(Initialvector.size() >= (128 / 8));
        assert(Cryptokey.size() >= (128 / 8));

        EVP_EncryptInit(Context, EVP_aes_128_cbc(), (const uint8_t *)Cryptokey.data(), (const uint8_t *)Initialvector.data());
        EVP_EncryptUpdate(Context, (uint8_t *)Buffer.get(), &Encryptionlength, (const uint8_t *)Plaintext.data(), int(Plaintext.size()));
        EVP_EncryptFinal_ex(Context, (uint8_t *)Buffer.get() + Encryptionlength, &Encryptionlength);
        EVP_CIPHER_CTX_free(Context);

        return std::basic_string<A>((A *)Buffer.get(), Encryptionlength);
    }
    template <typename A = uint8_t, Range_t B, Range_t C, Range_t D> std::basic_string<A> Encrypt_192(B &&Cryptokey, C &&Initialvector, D &&Plaintext)
    {
        const auto Buffer = std::make_unique<uint8_t []>(Plaintext.size() + ((Plaintext.size() & 15) == 0 ? 0 : (16 - (Plaintext.size() & 15))));
        EVP_CIPHER_CTX *Context = EVP_CIPHER_CTX_new();
        int Encryptionlength = 0;

        assert(Initialvector.size() >= (192 / 8));
        assert(Cryptokey.size() >= (192 / 8));

        EVP_EncryptInit(Context, EVP_aes_192_cbc(), (const uint8_t *)Cryptokey.data(), (const uint8_t *)Initialvector.data());
        EVP_EncryptUpdate(Context, (uint8_t *)Buffer.get(), &Encryptionlength, (const uint8_t *)Plaintext.data(), int(Plaintext.size()));
        EVP_EncryptFinal_ex(Context, (uint8_t *)Buffer.get() + Encryptionlength, &Encryptionlength);
        EVP_CIPHER_CTX_free(Context);

        return std::basic_string<A>((A *)Buffer.get(), Encryptionlength);
    }
    template <typename A = uint8_t, Range_t B, Range_t C, Range_t D> std::basic_string<A> Encrypt_256(B &&Cryptokey, C &&Initialvector, D &&Plaintext)
    {
        const auto Buffer = std::make_unique<uint8_t []>(Plaintext.size() + ((Plaintext.size() & 15) == 0 ? 0 : (16 - (Plaintext.size() & 15))));
        EVP_CIPHER_CTX *Context = EVP_CIPHER_CTX_new();
        int Encryptionlength = 0;

        assert(Initialvector.size() >= (256 / 8));
        assert(Cryptokey.size() >= (256 / 8));

        EVP_EncryptInit(Context, EVP_aes_256_cbc(), (const uint8_t *)Cryptokey.data(), (const uint8_t *)Initialvector.data());
        EVP_EncryptUpdate(Context, (uint8_t *)Buffer.get(), &Encryptionlength, (const uint8_t *)Plaintext.data(), int(Plaintext.size()));
        EVP_EncryptFinal_ex(Context, (uint8_t *)Buffer.get() + Encryptionlength, &Encryptionlength);
        EVP_CIPHER_CTX_free(Context);

        return std::basic_string<A>((A *)Buffer.get(), Encryptionlength);
    }

    template <typename A = uint8_t, Range_t B, Range_t C, Range_t D> std::basic_string<A> Decrypt_128(B &&Cryptokey, C &&Initialvector, D &&Ciphertext)
    {
        const auto Buffer = std::make_unique<uint8_t []>(Ciphertext.size() + ((Ciphertext.size() & 15) == 0 ? 0 : (16 - (Ciphertext.size() & 15))));
        EVP_CIPHER_CTX *Context = EVP_CIPHER_CTX_new();
        int Decryptionlength = 0;

        assert(Initialvector.size() >= (128 / 8));
        assert(Cryptokey.size() >= (128 / 8));

        EVP_DecryptInit(Context, EVP_aes_128_cbc(), (const uint8_t *)Cryptokey.data(), (const uint8_t *)Initialvector.data());
        EVP_DecryptUpdate(Context, (uint8_t *)Buffer.get(), &Decryptionlength, (const uint8_t *)Ciphertext.data(), int(Ciphertext.size()));
        EVP_DecryptFinal_ex(Context, (uint8_t *)Buffer.get() + Decryptionlength, &Decryptionlength);
        EVP_CIPHER_CTX_free(Context);

        return std::basic_string<A>((A *)Buffer.get(), Decryptionlength);
    }
    template <typename A = uint8_t, Range_t B, Range_t C, Range_t D> std::basic_string<A> Decrypt_192(B &&Cryptokey, C &&Initialvector, D &&Ciphertext)
    {
        const auto Buffer = std::make_unique<uint8_t []>(Ciphertext.size() + ((Ciphertext.size() & 15) == 0 ? 0 : (16 - (Ciphertext.size() & 15))));
        EVP_CIPHER_CTX *Context = EVP_CIPHER_CTX_new();
        int Decryptionlength = 0;

        assert(Initialvector.size() >= (192 / 8));
        assert(Cryptokey.size() >= (192 / 8));

        EVP_DecryptInit(Context, EVP_aes_192_cbc(), (const uint8_t *)Cryptokey.data(), (const uint8_t *)Initialvector.data());
        EVP_DecryptUpdate(Context, (uint8_t *)Buffer.get(), &Decryptionlength, (const uint8_t *)Ciphertext.data(), int(Ciphertext.size()));
        EVP_DecryptFinal_ex(Context, (uint8_t *)Buffer.get() + Decryptionlength, &Decryptionlength);
        EVP_CIPHER_CTX_free(Context);

        return std::basic_string<A>((A *)Buffer.get(), Decryptionlength);
    }
    template <typename A = uint8_t, Range_t B, Range_t C, Range_t D> std::basic_string<A> Decrypt_256(B &&Cryptokey, C &&Initialvector, D &&Ciphertext)
    {
        const auto Buffer = std::make_unique<uint8_t []>(Ciphertext.size() + ((Ciphertext.size() & 15) == 0 ? 0 : (16 - (Ciphertext.size() & 15))));
        EVP_CIPHER_CTX *Context = EVP_CIPHER_CTX_new();
        int Decryptionlength = 0;

        assert(Initialvector.size() >= (256 / 8));
        assert(Cryptokey.size() >= (256 / 8));

        EVP_DecryptInit(Context, EVP_aes_256_cbc(), (const uint8_t *)Cryptokey.data(), (const uint8_t *)Initialvector.data());
        EVP_DecryptUpdate(Context, (uint8_t *)Buffer.get(), &Decryptionlength, (const uint8_t *)Ciphertext.data(), int(Ciphertext.size()));
        EVP_DecryptFinal_ex(Context, (uint8_t *)Buffer.get(), &Decryptionlength);
        EVP_CIPHER_CTX_free(Context);

        return std::basic_string<A>((A *)Buffer.get(), Decryptionlength);
    }

    template <typename A = uint8_t, Range_t B, Range_t C> std::basic_string<A> Encrypt_128(B &&Cryptokey, C &&Plaintext) { return Encrypt_128(Cryptokey, Cryptokey, Plaintext); }
    template <typename A = uint8_t, Range_t B, Range_t C> std::basic_string<A> Encrypt_192(B &&Cryptokey, C &&Plaintext) { return Encrypt_192(Cryptokey, Cryptokey, Plaintext); }
    template <typename A = uint8_t, Range_t B, Range_t C> std::basic_string<A> Encrypt_256(B &&Cryptokey, C &&Plaintext) { return Encrypt_256(Cryptokey, Cryptokey, Plaintext); }

    template <typename A = uint8_t, Range_t B, Range_t C> std::basic_string<A> Decrypt_128(B &&Cryptokey, C &&Ciphertext) { return Decrypt_128(Cryptokey, Cryptokey, Ciphertext); }
    template <typename A = uint8_t, Range_t B, Range_t C> std::basic_string<A> Decrypt_192(B &&Cryptokey, C &&Ciphertext) { return Decrypt_192(Cryptokey, Cryptokey, Ciphertext); }
    template <typename A = uint8_t, Range_t B, Range_t C> std::basic_string<A> Decrypt_256(B &&Cryptokey, C &&Ciphertext) { return Decrypt_256<A>(Cryptokey, Cryptokey, Ciphertext); }
}

namespace DES3
{
    template <typename T> concept Range_t = requires (const T && t) { t.data(); t.size(); sizeof(t[0]) == 1; };

    template <typename A = uint8_t, Range_t B, Range_t C, Range_t D> std::basic_string<A> Encrypt(B &&Cryptokey, C &&Initialvector, D &&Plaintext)
    {
        const auto Buffer = std::make_unique<uint8_t []>(Plaintext.size() + ((Plaintext.size() & 15) == 0 ? 0 : (16 - (Plaintext.size() & 15))));
        EVP_CIPHER_CTX *Context = EVP_CIPHER_CTX_new();
        int Encryptionlength = 0;

        assert(Initialvector.size() >= (192 / 8));
        assert(Cryptokey.size() >= (192 / 8));

        EVP_EncryptInit(Context, EVP_des_ede3_cbc(), (const uint8_t *)Cryptokey.data(), (const uint8_t *)Initialvector.data());
        EVP_EncryptUpdate(Context, (uint8_t *)Buffer.get(), &Encryptionlength, (const uint8_t *)Plaintext.data(), int(Plaintext.size()));
        EVP_EncryptFinal_ex(Context, (uint8_t *)Buffer.get() + Encryptionlength, &Encryptionlength);
        EVP_CIPHER_CTX_free(Context);

        return std::basic_string<A>((A *)Buffer.get(), Encryptionlength);
    }
    template <typename A = uint8_t, Range_t B, Range_t C, Range_t D> std::basic_string<A> Decrypt(B &&Cryptokey, C &&Initialvector, D &&Ciphertext)
    {
        const auto Buffer = std::make_unique<uint8_t []>(Ciphertext.size() + ((Ciphertext.size() & 15) == 0 ? 0 : (16 - (Ciphertext.size() & 15))));
        EVP_CIPHER_CTX *Context = EVP_CIPHER_CTX_new();
        int Decryptionlength = 0;

        assert(Initialvector.size() >= (192 / 8));
        assert(Cryptokey.size() >= (192 / 8));

        EVP_DecryptInit(Context, EVP_des_ede3_cbc(), (const uint8_t *)Cryptokey.data(), (const uint8_t *)Initialvector.data());
        EVP_DecryptUpdate(Context, (uint8_t *)Buffer.get(), &Decryptionlength, (const uint8_t *)Ciphertext.data(), int(Ciphertext.size()));
        EVP_DecryptFinal_ex(Context, (uint8_t *)Buffer.get() + Decryptionlength, &Decryptionlength);
        EVP_CIPHER_CTX_free(Context);

        return std::basic_string<A>((A *)Buffer.get(), Decryptionlength);
    }

    template <typename A = uint8_t, Range_t B, Range_t C> std::basic_string<A> Encrypt(B &&Cryptokey, C &&Plaintext) { return Encrypt(Cryptokey, Cryptokey, Plaintext); }
    template <typename A = uint8_t, Range_t B, Range_t C> std::basic_string<A> Decrypt(B &&Cryptokey, C &&Ciphertext) { return Decrypt(Cryptokey, Cryptokey, Ciphertext); }
}

#endif
