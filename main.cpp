#include <QApplication>
#include <QWidget>
#include <QVBoxLayout>
#include <QTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QSocketNotifier>
#include <QMessageBox>

#include <iostream>
#include <string>
#include <mutex>
#include <atomic>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")
#define CLOSE_SOCKET closesocket
#define SOCKLEN int
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <errno.h>
#define CLOSE_SOCKET close
#define SOCKLEN socklen_t
#endif

#define DEFAULT_PORT "6667"
#define BUFFER_SIZE 512

std::mutex mtx;
std::atomic<bool> running(true);
int irc_socket = -1;

class IRCClient : public QObject {
    Q_OBJECT
public:
    IRCClient(const std::string& server, const std::string& port, const std::string& nick)
        : server(server), port(port), nick(nick) {}

    bool connect_to_server() {
#ifdef _WIN32
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            emit errorOccurred("WSAStartup failed: " + std::to_string(WSAGetLastError()));
            return false;
        }
#endif

        irc_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (irc_socket == -1) {
            emit errorOccurred("Socket creation failed");
            return false;
        }

        struct addrinfo hints{}, *result = nullptr;
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;

        if (getaddrinfo(server.c_str(), port.c_str(), &hints, &result) != 0) {
            emit errorOccurred("getaddrinfo failed");
            CLOSE_SOCKET(irc_socket);
            return false;
        }

        if (::connect(irc_socket, result->ai_addr, (SOCKLEN)result->ai_addrlen) == -1) {
            emit errorOccurred("Connect failed");
            freeaddrinfo(result);
            CLOSE_SOCKET(irc_socket);
            return false;
        }
        freeaddrinfo(result);

        send_command("NICK " + nick);
        send_command("USER " + nick + " 0 * :IRC Client");
        emit connected();
        return true;
    }

    void send_command(const std::string& command) {
        std::lock_guard<std::mutex> lock(mtx);
        std::string cmd = command + "\r\n";
        if (send(irc_socket, cmd.c_str(), cmd.length(), 0) == -1) {
            emit errorOccurred("Send failed");
        }
    }

    void start_receiving() {
        QSocketNotifier* notifier = new QSocketNotifier(irc_socket, QSocketNotifier::Read, this);
        connect(notifier, &QSocketNotifier::activated, this, &IRCClient::receive_message);
    }

private slots:
    void receive_message() {
        char buffer[BUFFER_SIZE];
        int bytes_received = recv(irc_socket, buffer, BUFFER_SIZE - 1, 0);
        if (bytes_received > 0) {
            buffer[bytes_received] = '\0';
            std::string message(buffer);
            emit messageReceived("[Server] " + message);
            if (message.substr(0, 4) == "PING") {
                std::string pong = "PONG " + message.substr(5);
                send_command(pong);
            }
        } else if (bytes_received == 0) {
            emit errorOccurred("Server closed connection.");
            running = false;
        } else {
#ifdef _WIN32
            int error = WSAGetLastError();
            if (error != WSAEWOULDBLOCK) {
                emit errorOccurred("Receive failed: " + std::to_string(error));
                running = false;
            }
#else
            if (errno != EWOULDBLOCK && errno != EAGAIN) {
                emit errorOccurred("Receive failed: " + std::to_string(errno));
                running = false;
            }
#endif
        }
    }

signals:
    void connected();
    void messageReceived(const std::string& message);
    void errorOccurred(const std::string& error);

private:
    std::string server, port, nick;
};

class MainWindow : public QWidget {
    Q_OBJECT
public:
    MainWindow(IRCClient* client) : client(client) {
        QVBoxLayout* layout = new QVBoxLayout(this);
        chat_display = new QTextEdit(this);
        chat_display->setReadOnly(true);
        input_field = new QLineEdit(this);
        send_button = new QPushButton("Send", this);

        layout->addWidget(chat_display);
        layout->addWidget(input_field);
        layout->addWidget(send_button);

        connect(send_button, &QPushButton::clicked, this, &MainWindow::send_input);
        connect(input_field, &QLineEdit::returnPressed, this, &MainWindow::send_input);
        connect(client, &IRCClient::messageReceived, this, &MainWindow::display_message);
        connect(client, &IRCClient::errorOccurred, this, &MainWindow::show_error);
        connect(client, &IRCClient::connected, this, &MainWindow::on_connected);
    }

private slots:
    void send_input() {
        std::string input = input_field->text().toStdString();
        input_field->clear();
        if (input.empty()) return;

        if (input == "QUIT") {
            client->send_command("QUIT :Goodbye");
            running = false;
            QApplication::quit();
        } else if (input.substr(0, 5) == "JOIN ") {
            client->send_command(input);
        } else if (input.substr(0, 5) == "NICK ") {
            client->send_command(input);
        } else if (input.substr(0, 4) == "MSG ") {
            size_t space = input.find(' ', 4);
            if (space != std::string::npos) {
                std::string target = input.substr(4, space - 4);
                std::string msg = input.substr(space + 1);
                client->send_command("PRIVMSG " + target + " :" + msg);
            } else {
                chat_display->append("Invalid MSG format. Use: MSG #channel message");
            }
        } else if (input.substr(0, 5) == "PART ") {
            client->send_command(input);
        } else {
            chat_display->append("Unknown command.");
        }
    }

    void display_message(const std::string& message) {
        chat_display->append(QString::fromStdString(message));
    }

    void show_error(const std::string& error) {
        chat_display->append("Error: " + QString::fromStdString(error));
    }

    void on_connected() {
        chat_display->append("Connected to IRC server.");
    }

private:
    IRCClient* client;
    QTextEdit* chat_display;
    QLineEdit* input_field;
    QPushButton* send_button;
};

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    std::string server, port, nick;
    std::cout << "Enter IRC server: ";
    std::getline(std::cin, server);
    std::cout << "Enter port (default 6667): ";
    std::getline(std::cin, port);
    if (port.empty()) port = DEFAULT_PORT;
    std::cout << "Enter nickname: ";
    std::getline(std::cin, nick);

    IRCClient client(server, port, nick);
    if (!client.connect_to_server()) {
        std::cerr << "Failed to connect to server." << std::endl;
        return 1;
    }

    MainWindow window(&client);
    window.show();

    client.start_receiving();

    return app.exec();
}

#include "main.moc" // Required for Qt meta-object compiler
