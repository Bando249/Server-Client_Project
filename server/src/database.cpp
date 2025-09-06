#include "../libs/sqlite3.h"
#include "../include/clientSession.h"
#include <filesystem>
#include <iostream>
#include <vector> 
#include <string>

using namespace std;

sqlite3 *initDatabase()
{
    string dbpath = "../data/server.db";

    filesystem::create_directories("../data");

    sqlite3 *db;
    int rc = sqlite3_open(dbpath.c_str(), &db);

    if (rc != SQLITE_OK)
    {
        cout << "Cant Init Database: " << sqlite3_errmsg(db) << endl;
        return nullptr;
    }

    const char *sqlCreateTables = R"(
    CREATE TABLE IF NOT EXISTS users(
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    username TEXT UNIQUE NOT NULL,
    password_hash TEXT NOT NULL
    );

    CREATE TABLE IF NOT EXISTS logs(
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    user_id INTEGER,
    action TEXT NOT NULL,
    timestamp DATETIME DEFAULT (datetime('now','localtime')),
    FOREIGN KEY(user_id) REFERENCES users(id)
    );

    CREATE TABLE IF NOT EXISTS msglogs(
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    user_id INTEGER,
    msg TEXT NOT NULL,
    timestamp DATETIME DEFAULT (datetime('now','localtime')),
    FOREIGN KEY(user_id) REFERENCES users(id)
    );

    CREATE TABLE IF NOT EXISTS files(
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    user_id INTEGER,
    file_name TEXT NOT NULL,
    file_path TEXT NOT NULL,
    timestamp DATETIME DEFAULT (datetime('now','localtime')),
    FOREIGN KEY(user_id) REFERENCES users(id)
    );
    )";
    

    char *errMsg = nullptr;
    rc = sqlite3_exec(db, sqlCreateTables, nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK)
    {
        cerr << "SQL error: " << errMsg << endl;
        sqlite3_free(errMsg);
        sqlite3_close(db);
        return nullptr;
    }

    return db;
}

bool insertIntoTable(sqlite3 *db,
                     const string &tableName,
                     const vector<string> &columns,
                     const vector<string> &values)
{
    if (columns.size() != values.size()) {
        cerr << "Columns and values size mismatch!" << endl;
        return false;
    }

    // Build the SQL with placeholders
    string sql = "INSERT INTO " + tableName + " (";
    for (size_t i = 0; i < columns.size(); i++) {
        sql += columns[i];
        if (i < columns.size() - 1) sql += ", ";
    }
    sql += ") VALUES (";
    for (size_t i = 0; i < values.size(); i++) {
        sql += "?";
        if (i < values.size() - 1) sql += ", ";
    }
    sql += ");";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << endl;
        return false;
    }

    // Bind values
    for (size_t i = 0; i < values.size(); i++) {
        sqlite3_bind_text(stmt, static_cast<int>(i + 1),
                          values[i].c_str(), -1, SQLITE_TRANSIENT);
    }

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        cerr << "Failed to execute statement: " << sqlite3_errmsg(db) << endl;
        sqlite3_finalize(stmt);
        return false;
    }

    sqlite3_finalize(stmt);
    return true;
}

void sendFileList(sqlite3 *db, clientSession &session) {
    const char *sql = "SELECT file_name FROM files";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        cerr << "Failed to prepare: " << sqlite3_errmsg(db) << endl;
        return;
    }

    std::string filelist;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const unsigned char *fileName = sqlite3_column_text(stmt, 0);
        if (fileName) {
            filelist += reinterpret_cast<const char *>(fileName);
            filelist += "\n";
        }
    }
    sqlite3_finalize(stmt);

    uint32_t len = filelist.size();
    send(session.socket, (char*)&len, sizeof(len), 0);
    send(session.socket, filelist.c_str(), len, 0);
}