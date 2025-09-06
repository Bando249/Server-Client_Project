#pragma once
#include <winsock2.h> // Core Winsock functions
#include <windows.h>
#include <string>
using namespace std;

struct clientSession
{
    SOCKET socket;
    int userId;
    string username;
    bool authenticated;

    clientSession(SOCKET s)
        : socket(s), userId(-1), username(""), authenticated(false) {}
};