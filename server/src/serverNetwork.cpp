#include <winsock2.h>
#include <thread>
#include <atomic>
#include <iostream>
#include "../include/serverHandles.h" // must come before usage
#include "../libs/sqlite3.h"
using namespace std;

void startServer(sqlite3 *db)
{
    // 1. Initialize Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        cerr << "WSAStartup failed!" << endl;
        return;
    }

    // 2. Create server socket
    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket == INVALID_SOCKET)
    {
        cerr << "Socket creation failed: " << WSAGetLastError() << endl;
        WSACleanup();
        return;
    }

    // 3. Bind socket to all interfaces
    sockaddr_in service;
    service.sin_family = AF_INET;
    service.sin_addr.s_addr = INADDR_ANY; // Listen on all network interfaces
    service.sin_port = htons(55555);

    if (bind(serverSocket, (SOCKADDR *)&service, sizeof(service)) == SOCKET_ERROR)
    {
        cerr << "Bind failed: " << WSAGetLastError() << endl;
        closesocket(serverSocket);
        sqlite3_close(db);
        WSACleanup();
        return;
    }

    // 4. Listen
    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR)
    {
        cerr << "Listen failed: " << WSAGetLastError() << endl;
        closesocket(serverSocket);
        sqlite3_close(db);
        WSACleanup();
        return;
    }

    cout << "Server listening on all network interfaces, port 55555..." << endl;

    // 5. Accept connections
    atomic<bool> serverRunning(true);
    while (serverRunning)
    {
        SOCKET clientSocket = accept(serverSocket, NULL, NULL);
        if (clientSocket == INVALID_SOCKET)
        {
            cerr << "Accept failed: " << WSAGetLastError() << endl;
            continue;
        }

        cout << "Accepted new client connection!" << endl;

        // Spawn thread to handle client
        thread(handle_client, clientSocket, db).detach();
    }

    // Cleanup
    closesocket(serverSocket);
    WSACleanup();
}

