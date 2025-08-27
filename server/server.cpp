#include <winsock2.h> // Core Winsock functions
#include <ws2tcpip.h> // Useful for getaddrinfo, inet_pton, etc.
#include <iostream>   // For printing/logging
#include <tchar.h>
#include <windows.h> // Required for SetConsoleTitle
#include <atomic>
#include <thread>

using namespace std;
bool Connected = true;

void handle_msg(SOCKET clientSocket)
{
    char recvBuffer[200];

    // Receive message from client
    int byteCount = recv(clientSocket, recvBuffer, sizeof(recvBuffer) - 1, 0);
    if (byteCount <= 0) {
        cout << "Client disconnected or recv failed: " << WSAGetLastError() << endl;
        return;
    }

    recvBuffer[byteCount] = '\0';
    cout << "Message Received: " << recvBuffer << endl;

    // Send confirmation
    const char* confirmation = "Server: Message Received\n";
    if (send(clientSocket, confirmation, strlen(confirmation), 0) == SOCKET_ERROR) {
        cout << "send() failed: " << WSAGetLastError() << endl;
    }
}

void handle_shutdown(SOCKET clientSocket)
{
    const char *shutdownMsg = "Shutting Down...";
    send(clientSocket, shutdownMsg, strlen(shutdownMsg), 0);
    cout << "shutting down client connection" << endl;
    Connected = false;
}

void handle_client(SOCKET clientSocket)
{

    char cmdBuffer[200];
    while (Connected)
    {

        int byteCount = recv(clientSocket, cmdBuffer, sizeof(cmdBuffer), 0);
        if (byteCount <= 0)
            break;
        cmdBuffer[byteCount] = '\0';

        if (strcmp(cmdBuffer, "shutdown") == 0)
        {
            handle_shutdown(clientSocket);
        }
        else if (strcmp(cmdBuffer, "msg") == 0)
        {
            handle_msg(clientSocket);
        }
    }

    closesocket(clientSocket);
    cout << "Client disconnected" << endl;
}



int main()
{
    SetConsoleTitle(TEXT("Radio Server"));

    // 1. Initialize WSA ------------ WSAStartup()

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        cout << "The Windsock dll not found!" << endl;
        return 0;
    }

    // 2. Create Socket ------------- socket()

    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket == INVALID_SOCKET)
    {
        cout << "Error at socket():" << WSAGetLastError() << endl;
        WSACleanup();
        return 0;
    }

    // 3. Bind Socket --------------- bind()

    sockaddr_in service;
    service.sin_family = AF_INET;
    InetPton(AF_INET, _T("127.0.0.1"), &service.sin_addr.s_addr);
    service.sin_port = htons(55555);

    if (bind(serverSocket, (SOCKADDR *)&service, sizeof(service)) == SOCKET_ERROR)
    {
        cout << "bind() failed: " << WSAGetLastError() << endl;
        closesocket(serverSocket);
        WSACleanup();
        return 0;
    }

    // 4. Listen on the Socket ------ listen()

    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR)
    {
        cout << "listen(): Error listening on socket" << WSAGetLastError() << endl;
        closesocket(serverSocket);
        WSACleanup();
        return 0;
    }
    cout << "Server Listening on Port 55555..." << endl;

    // 5. Accept a Connection ------- accept(), connect()

    
    atomic<bool> serverRunning(true);

    while (serverRunning)
    {
        SOCKET acceptSocket = accept(serverSocket, NULL, NULL);

        if (acceptSocket == INVALID_SOCKET)
        {
            cout << "Accept Failed: " << WSAGetLastError() << endl;
            continue;
        }

        cout << "Accepted Connection" << endl;

        thread(handle_client, acceptSocket).detach();
    }

    system("pause");
    closesocket(serverSocket);
    WSACleanup();

}