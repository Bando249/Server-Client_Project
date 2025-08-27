#include <winsock2.h> // Core Winsock functions
#include <ws2tcpip.h> // Useful for getaddrinfo, inet_pton, etc.
#include <iostream>   // For printing/logging
#include <tchar.h>
#include <windows.h> // Required for SetConsoleTitle

using namespace std;

int main()
{
    SetConsoleTitle(TEXT("Radio Server"));
    // 1. Initialize WSA ------------ WSAStartup()
    cout << "Initalizing WSA=======================" << endl;

    SOCKET serverSocket, acceptSocket;
    int port = 55555;
    WSADATA wsaData;
    int wsaerr;
    WORD wVersionRequested = MAKEWORD(2, 2);
    wsaerr = WSAStartup(wVersionRequested, &wsaData);
    if (wsaerr != 0)
    {
        cout << "The Windsock dll not found!" << endl;
        return 0;
    }
    else
    {
        cout << "The Windsock dll Found!" << endl;
        cout << "The Status: " << wsaData.szSystemStatus << endl;
    }

    // 2. Create Socket ------------- socket()
    cout << "Creating Socket===================" << endl;
    serverSocket = INVALID_SOCKET;
    serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket == INVALID_SOCKET)
    {
        cout << "Error at socket():" << WSAGetLastError() << endl;
        WSACleanup();
        return 0;
    }
    else
    {
        cout << "socket() is okay!" << endl;
    }
    // 3. Bind Socket --------------- bind()
    cout << "Attempting to bind Socket=======" << endl;
    sockaddr_in service;
    service.sin_family = AF_INET;
    InetPton(AF_INET, _T("127.0.0.1"), &service.sin_addr.s_addr);
    service.sin_port = htons(port);
    if (bind(serverSocket, (SOCKADDR *)&service, sizeof(service)) == SOCKET_ERROR)
    {
        cout << "bind() failed: " << WSAGetLastError() << endl;
        closesocket(serverSocket);
        WSACleanup();
        return 0;
    }
    else
    {
        cout << "Bind Succesful!" << endl;
    }
    // 4. Listen on the Socket ------ listen()
    cout << "Attempting to Listen==============" << endl;
    if (listen(serverSocket, 1) == SOCKET_ERROR)
    {
        cout << "listen(): Error listening on socket" << WSAGetLastError() << endl;
    }
    else
    {
        cout << "listen() success! Waiting For connections!" << endl;
    }
    // 5. Accept a Connection ------- accept(), connect()
    cout << "Accept a connection from client=============" << endl;
    acceptSocket = accept(serverSocket, NULL, NULL);
    if (acceptSocket == INVALID_SOCKET)
    {
        cout << "Accept Failed: " << WSAGetLastError() << endl;
        WSACleanup();
        return -1;
    }
    cout << "Accepted Conection" << endl;

    // 5. Send and Recieve Data ----- recv(), send(), recvfrom(), sendto()
    bool Connected = true;

    while (Connected)
    {
        char buffer[200];
        int byteCount = recv(acceptSocket, buffer, 200, 0);

        if (byteCount > 0)
        {
            cout << "Message Recieved: " << buffer << endl;
        }
        else
        {
            WSACleanup();
        }

        char confirmation[200] = "Server: Message Recieved!";
        byteCount = send(acceptSocket, confirmation, 200, 0);

        if (byteCount > 0)
        {
            cout << "Sent Conformation to client" << endl;
        }
        else
        {
            WSACleanup();
        }

        if (strcmp(buffer, "shutdown") == 0)
        {

            char shutdown[200] = "shutdown";
            byteCount = send(acceptSocket, shutdown, 200, 0);

            if (byteCount > 0)
            {
                cout << "sent shutdown command to client" << endl;
            }
            else
            {
                WSACleanup();
            }

            cout << "shutting down" << endl;
            Connected = false;
        }
    }
    // 6. Disconnect ---------------- closesocket()
    system("pause");
    WSACleanup();
}