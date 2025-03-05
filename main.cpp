#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <iostream>
#include <string>
#include <thread>
#include <mutex>
#include <atomic>
#include <vector>

#pragma comment(lib, "Ws2_32.lib")

#define DEFAULT_PORT "6667"
#define BUFFER_SIZE 512

std::mutex mtx;
std::atomic<bool> running(true);
SOCKET irc_socket = INVALID_SOCKET;

bool send_command(const std::string& command) {
    std::lock_guard<std::mutex> lock(mtx);
    std::string cmd = command + "\r\n";
    int bytes_sent = send(irc_socket, cmd.c_str(), cmd.length(), 0);
    if (bytes_sent == SOCKET_ERROR) {
        std::cerr << "Send failed: " << WSAGetLastError() << std::endl;
        return false;
    }
    return true;
}

void receive_messages() {
    std::string buffer;
    char temp_buf[BUFFER_SIZE];
    
    u_long mode = 1;
    ioctlsocket(irc_socket, FIONBIO, &mode);

    while (running) {
        int bytes_received = recv(irc_socket, temp_buf, BUFFER_SIZE - 1, 0);
        if (bytes_received > 0) {
            temp_buf[bytes_received] = '\0';
            buffer += temp_buf;

            size_t pos;
            while ((pos = buffer.find("\r\n")) != std::string::npos) {
                std::string message = buffer.substr(0, pos);
                buffer.erase(0, pos + 2);

                {
                    std::lock_guard<std::mutex> lock(mtx);
                    std::cout << "[Server] " << message << std::endl;
                }

                if (message.substr(0, 4) == "PING") {
                    std::string pong = "PONG " + message.substr(5);
                    send_command(pong);
                }
            }
        } else if (bytes_received == 0) {
            std::lock_guard<std::mutex> lock(mtx);
            std::cout << "Server closed connection." << std::endl;
            running = false;
            break;
        } else {
            int error = WSAGetLastError();
            if (error != WSAEWOULDBLOCK) {
                std::lock_guard<std::mutex> lock(mtx);
                std::cerr << "Receive failed: " << error << std::endl;
                running = false;
                break;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

bool connect_to_server(const std::string& server, const std::string& port, const std::string& nick) {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed: " << WSAGetLastError() << std::endl;
        return false;
    }

    irc_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (irc_socket == INVALID_SOCKET) {
        std::cerr << "Socket creation failed: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return false;
    }

    addrinfo hints{}, *result = nullptr;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    if (getaddrinfo(server.c_str(), port.c_str(), &hints, &result) != 0) {
        std::cerr << "getaddrinfo failed: " << WSAGetLastError() << std::endl;
        closesocket(irc_socket);
        WSACleanup();
        return false;
    }

    if (connect(irc_socket, result->ai_addr, (int)result->ai_addrlen) == SOCKET_ERROR) {
        std::cerr << "Connect failed: " << WSAGetLastError() << std::endl;
        freeaddrinfo(result);
        closesocket(irc_socket);
        WSACleanup();
        return false;
    }
    freeaddrinfo(result);

    send_command("NICK " + nick);
    send_command("USER " + nick + " 0 * :IRC Client");
    return true;
}

int main() {
    std::string server, port, nick;
    std::cout << "Enter IRC server: ";
    std::getline(std::cin, server);
    std::cout << "Enter port (default 6667): ";
    std::getline(std::cin, port);
    if (port.empty()) port = DEFAULT_PORT;
    std::cout << "Enter nickname: ";
    std::getline(std::cin, nick);

    if (!connect_to_server(server, port, nick)) {
        WSACleanup();
        return 1;
    }

    std::thread recv_thread(receive_messages);
    std::cout << "Commands: JOIN #channel, NICK newnick, MSG #channel message, PART #channel, QUIT\n";

    while (running) {
        std::string input;
        std::getline(std::cin, input);

        if (input.empty()) continue;

        if (input == "QUIT") {
            send_command("QUIT :Goodbye");
            running = false;
        } else if (input.substr(0, 5) == "JOIN ") {
            send_command(input);
        } else if (input.substr(0, 5) == "NICK ") {
            send_command(input);
        } else if (input.substr(0, 4) == "MSG ") {
            size_t space = input.find(' ', 4);
            if (space != std::string::npos) {
                std::string target = input.substr(4, space - 4);
                std::string msg = input.substr(space + 1);
                send_command("PRIVMSG " + target + " :" + msg);
            } else {
                std::cout << "Invalid MSG format. Use: MSG #channel message\n";
            }
        } else if (input.substr(0, 5) == "PART ") {
            send_command(input);
        } else {
            std::cout << "Unknown command.\n";
        }
    }

    recv_thread.join();
    closesocket(irc_socket);
    WSACleanup();
    return 0;
}
