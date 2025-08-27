#include <winsock2.h> // Core Winsock functions
#include <ws2tcpip.h> // Useful for getaddrinfo, inet_pton, etc.
#include <iostream>   // For printing/logging
#include <tchar.h>
#include <windows.h> // Required for SetConsoleTitle

using namespace std;

int main()
{
    SetConsoleTitle(TEXT("Radio Client"));
    // 1. Initialize WSA ------------ WSAStartup()
    SOCKET clientSocket;
    int port = 55555;
    WSADATA wsaData;

    int wsaerr;
    WORD wVersionRequested = MAKEWORD(2, 2);
    wsaerr = WSAStartup(wVersionRequested, &wsaData);

    if (wsaerr != 0)
    {
        cout << "The Winsock dll not found!" << endl;
        return 0;
    }
    else
    {
        cout << "The Winsockdll Found" << endl;
        cout << "The Status:" << wsaData.szSystemStatus << endl;
    }
    // 2. Create Socket ------------- socket()
    clientSocket = INVALID_SOCKET;
    clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
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
    clientService.sin_port = htons(port);
    if (connect(clientSocket, (SOCKADDR *)&clientService, sizeof(clientService)) == SOCKET_ERROR)
    {
        cout << "Client: connect() Failed to connect" << endl;
        WSACleanup();
        return 0;
    }
    else
    {
        cout << "Client: Connect() SUCCESS!" << endl;
        cout << "Client: Can Start sending and recieveing data..." << endl;
    }
    // 4. Send and Recieve Data
    bool Connected = true;

    while (Connected)
    {
        char buffer[200];
        cout << "Enter a message: ";
        cin.getline(buffer, 200);

        if (strcmp(buffer, "shutdown") == 0)
        {
            cout << " Shutting Down" << endl;
            Connected = false;
        }

        int byteCount = send(clientSocket, buffer, 200, 0);

        if (byteCount > 0)
        {
            cout << "Message Sent: " << buffer << endl;
        }
        else
        {
            WSACleanup;
        }

        byteCount = recv(clientSocket, buffer, 200, 0);

        if (byteCount > 0)
        {
            cout << buffer << endl;
        }
        else
        {
            WSACleanup;
        }
    }
    // 5. Close Socket
    system("pause");
    WSACleanup();
}