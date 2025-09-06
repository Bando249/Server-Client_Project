#pragma once
#include "sqlite3.h"
#include "clientSession.h"
#include <vector>
#include <string>

using namespace std;

void sendFileList(sqlite3  *db,clientSession &session);
bool insertIntoTable(sqlite3 *db,
                     const string &tableName,
                     const vector<std::string> &columns,
                     const vector<std::string> &values);
sqlite3* initDatabase();