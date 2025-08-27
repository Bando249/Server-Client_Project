#define NOMINMAX
#include <limits>
#include <winsock2.h> // Core Winsock functions
#include <ws2tcpip.h> // Useful for getaddrinfo, inet_pton, etc.
#include <iostream>   // For printing/logging
#include <tchar.h>
#include <windows.h> // Required for SetConsoleTitle
#include <string>

using namespace std;

void clientLoop(SOCKET clientSocket)
{
    bool connected = true;
    while (connected)
    {
        string command;
        cout << "Enter command (msg/file/shutdowm): ";
        cin >> command;
        cin.ignore(numeric_limits<streamsize>::max(), '\n'); // clear newline

        send(clientSocket, command.c_str(), command.size() + 1, 0);

        if (command == "msg")
        {
            string message;
            char buffer[200];

            cout << "Enter a message: ";
            getline(cin, message); // now safe

            send(clientSocket, message.c_str(), message.size() + 1, 0);
        }
        else if (command == "shutdown")
        {
            connected = false;
        }
    }
}

int main()
{
    SetConsoleTitle(TEXT("Radio Client"));
    // 1. Initialize WSA ------------ WSAStartup()
    WSADATA wsaData;

    if (WSAStartup(MAKEWORD(2, 2), &wsaData))
    {
        cout << "The Winsock dll not found!" << endl;
        return 0;
    }

    // 2. Create Socket ------------- socket()

    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (clientSocket == INVALID_SOCKET)
    {
        cout << "Error at socket" << WSAGetLastError << endl;
        WSACleanup();
        return 0;
    }
    else
    {
        cout << "socket() success!" << endl;
    }
    // 3. Connect to the server -------- connect()
    sockaddr_in clientService;
    clientService.sin_family = AF_INET;
    InetPton(AF_INET, _T("127.0.0.1"), &clientService.sin_addr.s_addr);
    clientService.sin_port = htons(55555);
    if (connect(clientSocket, (SOCKADDR *)&clientService, sizeof(clientService)) == SOCKET_ERROR)
    {
        cout << "Client: connect() Failed to connect" << endl;
        WSACleanup();
        return 0;
    }

    // 4. Send and Recieve Data

    clientLoop(clientSocket);

    // 5. Close Socket
    system("pause");
    closesocket(clientSocket);
    WSACleanup();
}