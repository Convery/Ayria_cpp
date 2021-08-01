/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2021-07-23
    License: MIT
*/

#include <Global.hpp>

namespace Networking
{
    static Response_t doRequest(std::string &&URL, std::string &&Data)
    {
        // No network available.
        if (Global.Settings.noNetworking) [[unlikely]] return { 500 };

        std::string Host{}, URI{}, Port{};
        std::smatch Match{};

        // Ensure proper formatting.
        if (!URL.starts_with("http")) URL.insert(0, "http://");

        // Extract the important parts from the URL.
        const std::regex RX(R"(\/\/([^\/:\s]+):?(\d+)?(.*))", std::regex_constants::optimize);
        if (!std::regex_match(URL, Match, RX) || Match.size() != 4) return { 500 };
        Host = Match[1].str(); Port = Match[2].str(); URI = Match[3].str();

        // Fallback if it's not available in the URL.
        const bool isHTTPS = URL.starts_with("https");
        if (Port.empty()) Port = isHTTPS ? "433" : "80";
        if (URI.empty()) URI = "/";

        // Totally legit HTTP..
        const auto Request = va("%s %s HTTP1.1\r\nContent-length: %zu\r\n\r\n%s",
            Data.empty() ? "GET" : "POST", URI.c_str(),
            Data.size(), Data.empty() ? "" : Data.c_str());

        // Common networking.
        addrinfo *Resolved{};
        HTTPResponse_t Parser{};
        size_t Socket = INVALID_SOCKET;
        const auto Buffer = alloca(4096);
        const addrinfo Hint{ .ai_family = AF_UNSPEC, .ai_socktype = SOCK_STREAM };
        const auto Cleanup = [&]()
        {
            if (Socket != INVALID_SOCKET) closesocket(Socket); Socket = INVALID_SOCKET;
            if (Resolved) freeaddrinfo(Resolved); Resolved = nullptr;
        };

        if (0 != getaddrinfo(Host.c_str(), Port.c_str(), &Hint, &Resolved)) { Cleanup(); return { 500 }; }
        Socket = socket(Resolved->ai_family, Resolved->ai_socktype, Resolved->ai_protocol);
        if (0 != connect(Socket, Resolved->ai_addr, (int)Resolved->ai_addrlen)) { Cleanup(); return { 500 }; }

        // SSL context needed.
        const auto doHTTPS = [&]() -> Response_t
        {
            const std::unique_ptr<SSL_CTX, decltype(&SSL_CTX_free)> Context(SSL_CTX_new(TLS_method()), SSL_CTX_free);
            SSL_CTX_set_max_proto_version(Context.get(), TLS1_3_VERSION);

            const std::unique_ptr<SSL, decltype(&SSL_free)> State(SSL_new(Context.get()), SSL_free);
            const auto Bio = BIO_new_socket(Socket, BIO_CLOSE);
            SSL_set_bio(State.get(), Bio, Bio);
            SSL_connect(State.get());

            // Because tools get upset about goto..
            while (true)
            {
                if (const auto Return = SSL_read(State.get(), Buffer, 4096); Return <= 0)
                {
                    if (SSL_ERROR_WANT_READ == SSL_get_error(State.get(), Return)) continue;
                    else { Cleanup(); return { 500 }; }
                }
                break;
            }
            while (true)
            {
                if (const auto Return = SSL_write(State.get(), Request.data(), Request.size()); Return <= 0)
                {
                    if (SSL_ERROR_WANT_WRITE == SSL_get_error(State.get(), Return)) continue;
                    else { Cleanup(); return { 500 }; }
                }
                break;
            }
            while (true)
            {
                const auto Return = SSL_read(State.get(), Buffer, 4096);
                if (0 == Return) { Cleanup(); return { Parser.Statuscode,  Parser.Body }; }

                if (Return < 0)
                {
                    if (SSL_ERROR_WANT_READ == SSL_get_error(State.get(), Return)) continue;
                    else break;
                }

                Parser.Parse(std::string_view(static_cast<char *>(Buffer), Return));
                if (Parser.isBad) break;
            }

            Cleanup();
            return { 500 };
        };
        const auto doHTTP = [&]() -> Response_t
        {
            if (const auto Return = send(Socket, Request.data(), Request.size(), NULL); Return <= 0)
            {
                Cleanup(); return { 500 };
            }

            while (true)
            {
                const auto Return = recv(Socket, static_cast<char *>(Buffer), 4096, NULL);
                if (0 == Return) { Cleanup();  return { Parser.Statuscode,  Parser.Body }; }
                if (Return < 0) break;

                Parser.Parse(std::string_view(static_cast<char *>(Buffer), Return));
                if (Parser.isBad) break;
            }

            Cleanup();
            return { 500 };
        };

        return isHTTPS ? doHTTPS() : doHTTP();
    }

    // Send a request and return the async response.
    std::future<Response_t> POST(std::string URL, std::string Data)
    {
        return std::async(std::launch::async, doRequest, std::move(URL), std::move(Data));
    }
    std::future<Response_t> GET(std::string URL)
    {
        return std::async(std::launch::async, doRequest, std::move(URL), std::string());
    }
}
