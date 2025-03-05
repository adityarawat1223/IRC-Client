# IRC Client

## Overview
This project is a simple IRC (Internet Relay Chat) client built using C++ and Qt. It allows users to connect to an IRC server, send commands, and participate in chat rooms.

## Features
- Connect to an IRC server
- Send and receive messages
- Join and leave channels
- Change nickname
- Simple GUI using Qt

## Requirements
### Dependencies
- Qt (QtWidgets, QtNetwork)
- C++17 or later

### Platform Support
- Windows (using Winsock2)
- Linux/macOS (using POSIX sockets)

## Installation and Setup
1. **Install Qt:** Ensure Qt is installed on your system.
2. **Clone the Repository:**
   ```sh
   git clone <repository-url>
   cd <repository-folder>
   ```
3. **Compile the Project:**
   ```sh
   qmake && make   # For Linux/macOS
   mingw32-make    # For Windows (using MinGW)
   ```
4. **Run the Executable:**
   ```sh
   ./IRCClient    # Linux/macOS
   IRCClient.exe  # Windows
   ```

## Usage
1. Launch the application.
2. Enter the IRC server, port (default: 6667), and nickname.
3. Use commands in the input field:
   - `JOIN #channel` - Join a channel
   - `PART #channel` - Leave a channel
   - `NICK newname` - Change nickname
   - `MSG #channel message` - Send a message to a channel
   - `QUIT` - Disconnect from the server

## Known Issues
- The application currently does not support SSL connections.
- Error handling can be improved for various socket failures.

## Future Enhancements
- Support for more IRC commands
- Improve UI/UX with better message formatting
- Add configuration options for user settings
- Implement channel user list display

## Contributing
Feel free to submit pull requests or report issues.

## License
This project is licensed under the MIT License.

---
**Author:** adityarawat1223 AKA burstingfire355





