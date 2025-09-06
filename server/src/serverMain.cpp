#include <iostream> 
#include "../include/database.h"
#include "../include/serverHandles.h"
#include "../include/serverNetwork.h"
#include <windows.h>
using namespace std;


int main()
{
    SetConsoleTitle(TEXT("Radio Server"));
    sqlite3* db = initDatabase();
    startServer(db);
    sqlite3_close(db);
}