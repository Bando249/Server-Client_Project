#define UNICODE
#define NOMINMAX
#include <limits>
#include <winsock2.h> // Core Winsock functions
#include <ws2tcpip.h> // Useful for getaddrinfo, inet_pton, etc.
#include <iostream>   // For printing/logging
#include <tchar.h>
#include <windows.h> // Required for SetConsoleTitle
#include <string>
#include <filesystem>
#include <fstream>
#include "network.h"

using namespace std;


int main()
{
    SetConsoleTitle(TEXT("Radio Client"));
    startServer();
}