#pragma once

#include <winsock2.h>   // for SOCKET
#include "sqlite3.h"

void handle_client(SOCKET clientSocket, sqlite3 *db);