#define UNICODE
#define NOMINMAX
#include <winsock2.h>
#include <limits>
// Core Winsock functions
#include <ws2tcpip.h> // Useful for getaddrinfo, inet_pton, etc.
#include <iostream>   // For printing/logging
#include <tchar.h>
#include <windows.h> // Required for SetConsoleTitle
#include <string>
#include <filesystem>
#include <fstream>
#include <vector>

using namespace std;
int recvAll(SOCKET sock, char *buf, int len)
{
    int total = 0;
    while (total < len - 1)
    { // leave space for null terminator
        int r = recv(sock, buf + total, len - 1 - total, 0);
        if (r <= 0)
        {
            // Handle internally: print and return special value
            std::cerr << "Socket disconnected or recv failed: " << WSAGetLastError() << std::endl;
            buf[0] = '\0'; // ensure buffer is safe
            return -1;     // sentinel value
        }
        total += r;
    }
    buf[total] = '\0';
    return total;
}
void handle_reg(SOCKET clientSocket)
{
    while (true)
    {
        string username;
        string password;

        cout << "Enter a username: ";
        getline(cin, username);

        send(clientSocket, username.c_str(), username.size() + 1, 0);

        cout << "Enter a password: ";
        getline(cin, password);

        send(clientSocket, password.c_str(), password.size() + 1, 0);

        char regBuff[200];
        int byteCount = recv(clientSocket, regBuff, sizeof(regBuff) - 1, 0);
        if (byteCount <= 0)
        {
            cout << "Client disconnected or recv failed: " << WSAGetLastError() << endl;
            return;
        }

        regBuff[byteCount] = '\0'; // null-terminate
        cout << regBuff << endl;

        if (string(regBuff).find("Registration successful") != string::npos)
            break;
    }
}
void handle_login(SOCKET clientSocket, bool &authenticated)
{
    while (true)
    {
        string username;
        string password;

        cout << "Enter Username: ";
        getline(cin, username);
        send(clientSocket, username.c_str(), username.size() + 1, 0);

        cout << "Enter Password: ";
        getline(cin, password);
        send(clientSocket, password.c_str(), password.size() + 1, 0);

        char loginBuff[200];
        int byteCount = recv(clientSocket, loginBuff, sizeof(loginBuff) - 1, 0);
        if (byteCount <= 0)
        {
            cout << "Server disconnected or recv failed: " << WSAGetLastError() << endl;
            return;
        }

        loginBuff[byteCount] = '\0';
        cout << loginBuff << endl;

        if (string(loginBuff).find("Login successful") != string::npos)
        {
            authenticated = true;
            break;
        }

        cout << "Try Again!" << endl;
    }
}
bool sendFile(SOCKET sock, const string &filePath)
{
    ifstream file(filePath, ios::binary | ios::ate);
    if (!file)
    {
        cout << "File not found!\n";
        return false;
    }

    streamsize fileSize = file.tellg();
    file.seekg(0, ios::beg);

    string fileName = filesystem::path(filePath).filename().string();

    uint32_t nameLen = fileName.size();
    send(sock, (char *)&nameLen, sizeof(nameLen), 0);
    send(sock, fileName.c_str(), nameLen, 0);

    int64_t size64 = fileSize;
    send(sock, (char *)&size64, sizeof(size64), 0);

    char buffer[4096];
    while (file)
    {
        file.read(buffer, sizeof(buffer));
        send(sock, buffer, file.gcount(), 0);
    }

    cout << "File sent!" << endl;
    return true;
}
bool requestFile(SOCKET socket, const string &saveDir)
{
    // Step 1: Prompt for filename
    string fileName;
    cout << "Enter the name of the file you want to download: ";
    cin >> fileName;
    cin.ignore(numeric_limits<streamsize>::max(), '\n');

    // Step 2: Send filename to server
    if (send(socket, fileName.c_str(), fileName.size(), 0) <= 0)
    {
        cerr << "Failed to send filename to server" << endl;
        return false;
    }

    // Step 3: Receive filename length
    uint32_t nameLen = 0;
    int ret = recv(socket, (char *)&nameLen, sizeof(nameLen), 0);
    if (ret <= 0 || nameLen == 0)
    {
        cerr << "Server indicates file not found or disconnected" << endl;
        return false;
    }

    // Step 4: Receive actual filename
    char nameBuf[256] = {0};
    ret = recv(socket, nameBuf, nameLen, 0);
    if (ret <= 0)
    {
        cerr << "Failed to receive filename from server" << endl;
        return false;
    }
    nameBuf[ret] = '\0';
    string serverFileName(nameBuf);

    // Step 5: Receive file size
    uint64_t fileSize = 0;
    ret = recv(socket, (char *)&fileSize, sizeof(fileSize), 0);
    if (ret <= 0 || fileSize == 0)
    {
        cerr << "Failed to receive file size" << endl;
        return false;
    }

    // Step 6: Open file for writing
    filesystem::path outPath = filesystem::path(saveDir) / serverFileName;
    filesystem::create_directories(outPath.parent_path());


    ofstream outFile(saveDir + "/" + serverFileName, ios::binary);
    if (!outFile)
    {
        cerr << "Failed to open file for writing" << endl;
        return false;
    }

    // Step 7: Receive file contents
    char buffer[4096];
    uint64_t received = 0;
    while (received < fileSize)
    {
        int toRead = min((uint64_t)sizeof(buffer), fileSize - received);
        int byteCount = recv(socket, buffer, toRead, 0);
        if (byteCount <= 0)
        {
            cerr << "File transfer interrupted or client disconnected" << endl;
            return false;
        }
        outFile.write(buffer, byteCount);
        received += byteCount;
    }

    cout << "File received: " << serverFileName << endl;
    return true;
}
void clientLoop(SOCKET clientSocket)
{
    bool loggedIn = false;
    bool connected = true;

    while (connected)
    {
        string inputPrompt = loggedIn ? "Enter command (msg/sendfile/viewfiles/downloadfile/shutdown): " : "Enter command (login/register): ";
        cout << inputPrompt;

        string command;
        cin >> command;
        cin.ignore(numeric_limits<streamsize>::max(), '\n');

        if (loggedIn)
        {
            if (command == "msg")
            {
                send(clientSocket, command.c_str(), command.size() + 1, 0);

                string message;
                cout << "Enter a message: ";
                getline(cin, message);
                send(clientSocket, message.c_str(), message.size() + 1, 0);

                char msgBuff[200];
                int byteCount = recv(clientSocket, msgBuff, sizeof(msgBuff) - 1, 0);
                if (byteCount > 0)
                {
                    msgBuff[byteCount] = '\0';
                    cout << msgBuff << endl;
                }
            }
            else if (command == "shutdown")
            {
                send(clientSocket, command.c_str(), command.size() + 1, 0);
                connected = false;
            }
            else if (command == "sendfile")
            {
                string localFile;
                cout << "Enter path to the file you want to send (e.g., C:/Users/user/Desktop/file.ext): ";
                getline(cin, localFile);

                // Check if the file exists first
                if (!filesystem::exists(localFile))
                {
                    cout << "File not found! Command aborted." << endl;
                    continue; // do not send anything to server
                }

                // Only send command if file exists
                send(clientSocket, command.c_str(), command.size() + 1, 0);

                bool success = sendFile(clientSocket, localFile);
                if (success)
                {
                    cout << "File sent successfully" << endl;
                }
                else
                {
                    cout << "File transfer failed!" << endl;
                }
            }
            else if (command == "viewfiles")
            {
                send(clientSocket, command.c_str(), command.size() + 1, 0);

                uint32_t len = 0;
                if (recv(clientSocket, (char *)&len, sizeof(len), 0) <= 0)
                {
                    cout << "Failed to read length or server disconnected\n";
                    break;
                }

                vector<char> buffer(len + 1);
                int received = 0;
                while (received < len)
                {
                    int r = recv(clientSocket, buffer.data() + received, len - received, 0);
                    if (r <= 0)
                    {
                        cout << "Server disconnected or recv failed: " << WSAGetLastError() << endl;
                        break;
                    }
                    received += r;
                }

                buffer[len] = '\0';
                cout << "Files:\n"
                     << buffer.data() << endl;
            }
            else if (command == "downloadfile")
            {
                // Step 1: Notify server of download command
                send(clientSocket, command.c_str(), command.size() + 1, 0);

                // Step 2: Attempt to request file
                bool success = requestFile(clientSocket, "../../../clientsFolder");

                // Step 3: Check result
                if (success)
                {
                    cout << "Download completed successfully.\n";
                }
                else
                {
                    cout << "Download failed or file not found.\n";
                }
            }
            else
            {
                cout << "Invalid command. Try again!\n";
            }
        }
        else
        {
            if (command == "login")
            {
                send(clientSocket, command.c_str(), command.size() + 1, 0);
                handle_login(clientSocket, loggedIn);
            }
            else if (command == "register")
            {
                send(clientSocket, command.c_str(), command.size() + 1, 0);
                handle_reg(clientSocket);
            }
            else
            {
                cout << "Invalid command. Try again!\n";
            }
        }
    }
}
