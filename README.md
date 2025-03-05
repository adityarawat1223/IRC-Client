# IRC Client

A simple IRC (Internet Relay Chat) client built in C++ using Windows Sockets (Winsock). This client allows users to connect to an IRC server, send messages, change nicknames, and join or leave channels.

## Features
- Connect to any IRC server
- Send and receive messages
- Join and leave channels
- Change nickname dynamically
- Gracefully handle server disconnects

## Requirements
- Windows OS
- Visual Studio or any C++ compiler supporting Winsock

## Installation & Compilation

### **Using Visual Studio**
1. Open Visual Studio and create a new **Console Application**.
2. Copy and paste the source code into `main.cpp`.
3. Link with `Ws2_32.lib` (already handled by `#pragma comment(lib, "Ws2_32.lib")`).
4. Build and run the application.

### **Using g++ (MinGW on Windows)**
If you're using MinGW, you can compile it using:
```sh
 g++ -o irc_client irc_client.cpp -lws2_32
```

## Usage

1. Run the executable:
   ```sh
   ./irc_client.exe
   ```
2. Enter the IRC server address.
3. Enter the port (default is `6667`).
4. Enter a nickname.
5. Use the available commands:
   - `JOIN #channel` → Join an IRC channel.
   - `NICK newnick` → Change your nickname.
   - `MSG #channel message` → Send a message to a channel.
   - `PART #channel` → Leave a channel.
   - `QUIT` → Disconnect from the server.

## Example Session
```
Enter IRC server: irc.freenode.net
Enter port (default 6667): 6667
Enter nickname: MyNick
Commands: JOIN #channel, NICK newnick, MSG #channel message, PART #channel, QUIT
> JOIN #general
> MSG #general Hello everyone!
> NICK NewNick
> QUIT
```

## How It Works
- The client connects to an IRC server using **Winsock**.
- A separate thread continuously listens for incoming messages from the server.
- User input is processed and converted into IRC commands.
- Messages are sent using `send()` and received using `recv()`.
- Handles **PING** messages from the server by responding with **PONG**.

## Future Improvements
- Support for SSL/TLS connections.
- Improved command parsing.
- GUI version of the client.

## License
This project is licensed under the MIT License.

## Contributions
Contributions are welcome! Feel free to fork and submit pull requests.

---
**Author:** [adityarawat1223]  


