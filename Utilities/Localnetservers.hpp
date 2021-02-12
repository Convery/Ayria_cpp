/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-02-12
    License: MIT
*/

#pragma once
#include <Stdinclude.hpp>

#pragma pack(push, 1)
struct Address_t { unsigned int IPv4; unsigned short Port; };
using Keyval = std::pair<std::string, std::string>;
struct HTTPRequest_t
{
    bool Parsed;
    std::string URL;
    std::string Body;
    std::string Method;
    std::vector<Keyval> Headers;
};
#pragma pack(pop)

// Server interfaces for localnetworking-plugins.
// Callbacks return false on error or if there's no data.
struct IServer
{
    // No complaints.
    virtual ~IServer() = default;

    // Utility functionality.
    virtual void onConnect() {}
    virtual void onDisconnect() {}

    // Stream-based IO for protocols such as TCP.
    virtual bool onStreamread(void *Databuffer, unsigned int *Datasize) = 0;
    virtual bool onStreamwrite(const void *Databuffer, unsigned int Datasize) = 0;

    // Packet-based IO for protocols such as UDP and ICMP.
    virtual bool onPacketread(void *Databuffer, unsigned int *Datasize) = 0;
    virtual bool onPacketwrite(const void *Databuffer, unsigned int Datasize, const Address_t *Endpoint) = 0;
};
struct IStreamserver : IServer
{
    // Nullsub packet-based IO.
    bool onPacketread(void *, unsigned int *) override { return false; }
    bool onPacketwrite(const void *, unsigned int, const Address_t *) override { return false; }
};
struct IDatagramserver : IServer
{
    // Nullsub stream-based IO.
    bool onStreamread(void *, unsigned int *) override { return false; }
    bool onStreamwrite(const void *, unsigned int) override { return false; }
};

// HTTP parsing.
#if defined(HAS_HTTPPARSER)
// The HTTP server just decodes the data and pass it to a callback.
struct IHTTPServer : IStreamserver
{
    // HTTP parser information.
    http_parser_settings Settings{};
    HTTPRequest_t Parsedrequest{};
    std::string StreamOUT{};
    http_parser Parser{};

    // Callbacks on data, returns the response.
    std::string(__cdecl *onGET)(HTTPRequest_t &){};
    std::string(__cdecl *onPUT)(HTTPRequest_t &){};
    std::string(__cdecl *onPOST)(HTTPRequest_t &){};
    std::string(__cdecl *onCOPY)(HTTPRequest_t &){};
    std::string(__cdecl *onDELETE)(HTTPRequest_t &){};

    // On incoming data from Localnet.
    bool onStreamwrite(const void *Databuffer, const uint32_t Datasize) override
    {
        assert(Databuffer); assert(Datasize);

        // Clear any old data.
        if (Parsedrequest.Parsed)
        {
            Parsedrequest.URL.clear();
            Parsedrequest.Body.clear();
            Parsedrequest.Method.clear();
            Parsedrequest.Headers.clear();
            Parsedrequest.Parsed = false;
        }

        // Parse the incoming data.
        http_parser_execute(&Parser, &Settings, (const char *)Databuffer, Datasize);

        // Forward to the callbacks if parsed.
        if (Parsedrequest.Parsed)
        {
            std::string Result{};
            switch (Hash::WW32(Parsedrequest.Method.c_str()))
            {
                case Hash::WW32("GET"): if (onGET) Result = onGET(Parsedrequest); break;
                case Hash::WW32("PUT"): if (onPUT) Result = onPUT(Parsedrequest); break;
                case Hash::WW32("POST"): if (onPOST) Result = onPOST(Parsedrequest); break;
                case Hash::WW32("COPY"): if (onCOPY) Result = onCOPY(Parsedrequest); break;
                case Hash::WW32("DELETE"): if (onDELETE) Result = onDELETE(Parsedrequest); break;
                default:
                    Errorprint(va("HTTP server got a request for %s, but there's no handler for the type.", Parsedrequest.URL.c_str()).c_str());
                    return true;
            }
            StreamOUT += {Result.begin(), Result.end()};
        }

        return true;
    }

    // On outgoing data to Localnet.
    bool onStreamread(void *Databuffer, uint32_t *Datasize) override
    {
        assert(Databuffer); assert(Datasize);
        if (StreamOUT.empty()) return false;

        const auto Size = std::min(*Datasize, uint32_t(StreamOUT.size()));
        std::memcpy(Databuffer, StreamOUT.data(), Size);
        StreamOUT.erase(0, Size);
        *Datasize = Size;
        return true;
    }

    // Initialize on startup.
    IHTTPServer()
    {
        http_parser_init(&Parser, HTTP_REQUEST);
        http_parser_settings_init(&Settings);
        Parser.data = &Parsedrequest;

        Settings.on_message_begin = [](http_parser *) -> int
        {
            return 0;
        };
        Settings.on_headers_complete = [](http_parser *Parser) -> int
        {
            ((HTTPRequest_t *)Parser->data)->Method = http_method_str(http_method(Parser->method));
            return 0;
        };
        Settings.on_message_complete = [](http_parser *Parser) -> int
        {
            ((HTTPRequest_t *)Parser->data)->Parsed = true;
            return 0;
        };
        Settings.on_url = [](http_parser *Parser, const char *Data, const size_t Length) -> int
        {
            ((HTTPRequest_t *)Parser->data)->URL = std::string(Data, Length);
            return 0;
        };
        Settings.on_body = [](http_parser *Parser, const char *Data, const size_t Length) -> int
        {
            ((HTTPRequest_t *)Parser->data)->Body += std::string(Data, Length);
            return 0;
        };
        Settings.on_header_field = [](http_parser *Parser, const char *Data, const size_t Length) -> int
        {
            ((HTTPRequest_t *)Parser->data)->Headers.emplace_back().first = std::string(Data, Length);
            return 0;
        };
        Settings.on_header_value = [](http_parser *Parser, const char *Data, const size_t Length) -> int
        {
            ((HTTPRequest_t *)Parser->data)->Headers.back().second = std::string(Data, Length);
            return 0;
        };
    }
};

#if defined(HAS_OPENSSL)
#include <openssl/err.h>
struct IHTTPSServer : IHTTPServer
{
    // SSL information.
    X509 *SSLCertificate{};
    EVP_PKEY *SSLKey{};
    SSL_CTX *Context{};
    BIO *Write_BIO{};
    BIO *Read_BIO{};
    SSL *State{};

    // Callback on incoming data.
    bool onStreamwrite(const void *Databuffer, const uint32_t Datasize) override
    {
        assert(Databuffer); assert(Datasize);

        // OpenSSL seems to prefer working on BIOs.
        BIO_write(Read_BIO, Databuffer, Datasize);

        // Automatically pushes to Write_BIO.
        if (!SSL_is_init_finished(State))
        {
            SSL_do_handshake(State);
            return true;
        }

        // Work in smaller batches.
        uint8_t Buffer[2048]; int Readcount;
        do
        {
            Readcount = SSL_read(State, Buffer, 2048);

            // Check errors.
            if (Readcount <= 0)
            {
                const size_t Error = SSL_get_error(State, 0);
                if (Error == SSL_ERROR_ZERO_RETURN)
                {
                    // Remake the SSL state.
                    {
                        SSL_free(State);

                        Write_BIO = BIO_new(BIO_s_mem());
                        Read_BIO = BIO_new(BIO_s_mem());
                        BIO_set_nbio(Write_BIO, 1);
                        BIO_set_nbio(Read_BIO, 1);

                        State = SSL_new(Context);
                        if (!State) Errorprint("OpenSSL error: Failed to create the SSL state.");

                        SSL_set_verify(State, SSL_VERIFY_NONE, nullptr);
                        SSL_set_bio(State, Read_BIO, Write_BIO);
                        SSL_set_accept_state(State);
                    }

                }

                return false;
            }

            // Let the base parse this data.
            IHTTPServer::onStreamwrite(Buffer, Readcount);
            if (!StreamOUT.empty()) SSL_write(State, StreamOUT.data(), int(StreamOUT.size()));
            StreamOUT.clear();
        }
        while (Readcount);

        return true;
    }
    bool onStreamread(void *Databuffer, uint32_t *Datasize) override
    {
        assert(Databuffer); assert(Datasize);
        const auto Readcount = BIO_read(Write_BIO, Databuffer, *Datasize);
        *Datasize = Readcount;
        return Readcount > 0;
    }

    // Initialize on startup.
    IHTTPSServer(const std::string_view Hostname) : IHTTPServer()
    {
        bool Certcreated = false;
        do
        {
            // Ensure that we are initialized.
            OpenSSL_add_ssl_algorithms();
            SSL_load_error_strings();

            // Allocate the PKEY.
            SSLKey = EVP_PKEY_new();
            if (!SSLKey) break;

            // Create the RSA key.
            RSA *RSAKey = RSA_new();
            BIGNUM *Exponent = BN_new();
            if (!BN_set_word(Exponent, 65537)) break;
            if (!RSA_generate_key_ex(RSAKey, 2048, Exponent, nullptr)) break;
            if (!EVP_PKEY_assign_RSA(SSLKey, RSAKey)) break;

            // Allocate the x509 cert.
            SSLCertificate = X509_new();
            if (!SSLCertificate) break;

            // Cert details, valid for a year.
            X509_gmtime_adj(X509_get_notBefore(SSLCertificate), 0);
            ASN1_INTEGER_set(X509_get_serialNumber(SSLCertificate), 1);
            X509_gmtime_adj(X509_get_notAfter(SSLCertificate), 31536000L);

            // Set the public key.
            X509_set_pubkey(SSLCertificate, SSLKey);

            // Name information.
            X509_NAME *Name = X509_get_subject_name(SSLCertificate);
            X509_NAME_add_entry_by_txt(Name, "C", MBSTRING_ASC, (unsigned char *)"SE", -1, -1, 0);
            X509_NAME_add_entry_by_txt(Name, "O", MBSTRING_ASC, (unsigned char *)"Hedgehogscience", -1, -1, 0);
            X509_NAME_add_entry_by_txt(Name, "CN", MBSTRING_ASC, (unsigned char *)Hostname.data(), int(Hostname.size()), -1, 0);

            // Set the issuer name.
            X509_set_issuer_name(SSLCertificate, Name);

            // Sign the certificate with the key.
            if (!X509_sign(SSLCertificate, SSLKey, EVP_sha1()))
            {
                X509_free(SSLCertificate);
                SSLCertificate = nullptr;
                EVP_PKEY_free(SSLKey);
                SSLKey = nullptr;
                break;
            }

            Certcreated = true;
        }
        while (false);

        if (!Certcreated)
        {
            Errorprint(va("Failed to create a SSL certificate for \"%*s\"", Hostname.size(), Hostname.data()).c_str());
            SSLCertificate = nullptr;
            SSLKey = nullptr;
            return;
        }

        // Initialize the context.
        {
            Context = SSL_CTX_new(SSLv23_server_method());
            SSL_CTX_set_verify(Context, SSL_VERIFY_NONE, nullptr);

            SSL_CTX_set_options(Context, SSL_OP_SINGLE_DH_USE);
            SSL_CTX_set_ecdh_auto(Context, 1);

            uint8_t ssl_context_id[16]{ 2, 3, 4, 5, 6 };
            SSL_CTX_set_session_id_context(Context, (const unsigned char *)&ssl_context_id, sizeof(ssl_context_id));
        }

        // Load the certificate and key for this server.
        {
            auto Resultcode = SSL_CTX_use_certificate(Context, SSLCertificate);
            if (Resultcode != 1) Errorprint(va("OpenSSL error: %s", ERR_error_string(Resultcode, nullptr)).c_str());

            Resultcode = SSL_CTX_use_PrivateKey(Context, SSLKey);
            if (Resultcode != 1) Errorprint(va("OpenSSL error: %s", ERR_error_string(Resultcode, nullptr)).c_str());

            Resultcode = SSL_CTX_check_private_key(Context);
            if (Resultcode != 1) Errorprint(va("OpenSSL error: %s", ERR_error_string(Resultcode, nullptr)).c_str());
        }

        // Create the BIO buffers.
        {
            Write_BIO = BIO_new(BIO_s_mem());
            Read_BIO = BIO_new(BIO_s_mem());
            BIO_set_nbio(Write_BIO, 1);
            BIO_set_nbio(Read_BIO, 1);
        }

        // Initialize the SSL state.
        {
            State = SSL_new(Context);
            if (!State)
            {
                Errorprint("OpenSSL error: Failed to create the SSL state.");
                SSLCertificate = nullptr;
                SSLKey = nullptr;
                return;
            }

            SSL_set_verify(State, SSL_VERIFY_NONE, nullptr);
            SSL_set_bio(State, Read_BIO, Write_BIO);
            SSL_set_accept_state(State);
        }
    }
};
#endif
#endif
