#include <winsock2.h>

#include "../libs/sqlite3.h"
#include "../include/clientSession.h"
#include "../include/database.h"
#include <fstream>   // add this at the top
#include <cstdint>
#include <iostream>
#include <filesystem>
using namespace std;

void handle_reg(clientSession &session, sqlite3 *db)
{
    char userBuff[200];
    char pasBuff[200];

    while (true)
    {
        // Receive username
        int byteCount = recv(session.socket, userBuff, sizeof(userBuff) - 1, 0);
        if (byteCount <= 0) return;
        userBuff[byteCount] = '\0';

        // Receive password
        byteCount = recv(session.socket, pasBuff, sizeof(pasBuff) - 1, 0);
        if (byteCount <= 0) return;
        pasBuff[byteCount] = '\0';

        // Insert user into 'users' table
        vector<string> userCols = {"username", "password_hash"};
        vector<string> userVals = {userBuff, pasBuff};

        if (!insertIntoTable(db, "users", userCols, userVals))
        {
            const char* failMsg = "Failed to register user. Try again.";
            send(session.socket, failMsg, strlen(failMsg), 0);
            continue;
        }
        else{
        // Get the auto-generated user_id
        sqlite3_int64 newUserId = sqlite3_last_insert_rowid(db);
        session.userId = (newUserId); 

        // Log the registration
        vector<string> logCols = {"user_id", "action"};
        vector<string> logVals = {to_string(session.userId), "Registered"};
        insertIntoTable(db, "logs", logCols, logVals); 

        // Send success message
        const char* successMsg = "Registration successful";
        send(session.socket, successMsg, strlen(successMsg), 0);
        break; 
        }
    }
}
void handle_login(clientSession &session, sqlite3 *db)
{
    char userBuff[200];
    char passBuff[200];

    while (true)
    {
        // Receive username
        int byteCount = recv(session.socket, userBuff, sizeof(userBuff) - 1, 0);
        if (byteCount <= 0)
            return;
        userBuff[byteCount] = '\0';
        // Receive password
        byteCount = recv(session.socket, passBuff, sizeof(passBuff) - 1, 0);
        if (byteCount <= 0)
            return;
        passBuff[byteCount] = '\0';

        cout << "Login Attempt: " << userBuff << endl;

        const char *sql = "SELECT id, password_hash FROM users WHERE username = ?;";
        sqlite3_stmt *stmt;

        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        {
            cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << endl;
            return;
        }

        sqlite3_bind_text(stmt, 1, userBuff, -1, SQLITE_TRANSIENT);

        int rc = sqlite3_step(stmt);
        if (rc == SQLITE_ROW)
        {
            int userId = sqlite3_column_int(stmt, 0); // ✅ correct column index
            const unsigned char *storedPass = sqlite3_column_text(stmt, 1);

            if (storedPass && strcmp((const char *)storedPass, passBuff) == 0)
            {
                session.userId = userId;
                session.username = userBuff;
                session.authenticated = true;

                vector<string> logCols = {"user_id", "action"};
                vector<string> logVals = {to_string(session.userId), "Login"};
                insertIntoTable(db, "logs", logCols, logVals);
                string success = "Login successful. User ID: " + to_string(userId) + "\n";
                send(session.socket, success.c_str(), success.size(), 0);
                cout << "User logged in successfully: " << userBuff << " | ID: " << userId << endl;
                sqlite3_finalize(stmt);
                break;
            }
            else
            {
                const char *failMsg = "Login Failed: Invalid password\n";
                send(session.socket, failMsg, strlen(failMsg), 0);
                vector<string> logCols = {"user_id", "action"};
                vector<string> logVals = {"-1", "Failed Login"};
                insertIntoTable(db, "logs", logCols, logVals);
            }
        }
        else
        {
            const char *failMsg = "Login Failed: Username not found\n";
            send(session.socket, failMsg, strlen(failMsg), 0);
            vector<string> logCols = {"user_id", "action"};
            vector<string> logVals = {"-1", "Failed Login"};
            insertIntoTable(db, "logs", logCols, logVals);
        }

        sqlite3_finalize(stmt);
    }
}
void handle_msg(clientSession &session, sqlite3 *db)
{
    char recvBuffer[200];

    // Receive message from client
    int byteCount = recv(session.socket, recvBuffer, sizeof(recvBuffer) - 1, 0);
    if (byteCount <= 0)
    {
        cout << "Client disconnected or recv failed: " << WSAGetLastError() << endl;
        return;
    }

    recvBuffer[byteCount] = '\0';
    cout << session.username << " (" << session.userId << "): " << recvBuffer << endl;

    // Send confirmation
    const char *confirmation = "Server: Message Received\n";
    if (send(session.socket, confirmation, strlen(confirmation) +1, 0) == SOCKET_ERROR)
    {
        cout << "send() failed: " << WSAGetLastError() << endl;
    }
    else
    {        
        vector<string> msgCols = {"user_id", "msg"};
        vector<string> msgVals = {to_string(session.userId), recvBuffer};
        insertIntoTable(db, "msglogs", msgCols, msgVals);
        vector<string> logCols = {"user_id", "action"};
        vector<string> logVals = {to_string(session.userId), "Message Sent"};
        insertIntoTable(db, "logs", logCols, logVals);
    }
}
void handle_shutdown(clientSession &session, bool &connected)
{
    const char *shutdownMsg = "Shutting Down...";
    send(session.socket, shutdownMsg, strlen(shutdownMsg), 0);
    cout << "shutting down client connection" << endl;
    connected = false;
}
bool sendFile(clientSession &session, sqlite3 *db) {
    // Step 1: Receive requested filename from client
    char nameBuf[256] = {0};
    int ret = recv(session.socket, nameBuf, sizeof(nameBuf) - 1, 0);
    if (ret <= 0) {
        cerr << "Failed to receive filename from client" << endl;
        return false;
    }
    nameBuf[ret] = '\0';  // null terminate
    string requestedName(nameBuf);

    // Step 2: Construct full file path
    string filePath = "../fileRepo/" + requestedName;

    // Step 3: Open file
    ifstream file(filePath, ios::binary | ios::ate);
    if (!file) {
        cerr << "File not found: " << filePath << endl;
        // Send 0 length to indicate file not found
        uint32_t nameLen = 0;
        send(session.socket, (char*)&nameLen, sizeof(nameLen), 0);
        return false;
    }

    streamsize fileSize = file.tellg();
    file.seekg(0, ios::beg);

    // ✅ Use base filename
    string fileName = filesystem::path(filePath).filename().string();
    uint32_t nameLen = static_cast<uint32_t>(fileName.size());

    // Step 4: Send filename length & filename
    send(session.socket, (char*)&nameLen, sizeof(nameLen), 0);
    send(session.socket, fileName.c_str(), nameLen, 0);

    // Step 5: Send file size
    int64_t size64 = fileSize;
    send(session.socket, (char*)&size64, sizeof(size64), 0);

    // Step 6: Send file contents
    char buffer[4096];
    while (file) {
        file.read(buffer, sizeof(buffer));
        send(session.socket, buffer, file.gcount(), 0);
    }

    cout << "File sent: " << fileName << endl;

    // Log download
    vector<string> logCols = {"user_id", "action"};
    vector<string> logVals = {to_string(session.userId), "file downloaded"};
    insertIntoTable(db, "logs", logCols, logVals);

    return true;
}
bool receiveFile(clientSession &session, const string &saveDir, sqlite3 *db) {
    uint32_t nameLen = 0;
    int ret = recv(session.socket, (char*)&nameLen, sizeof(nameLen), 0);
    if (ret <= 0 || nameLen == 0 || nameLen >= 256) {
        cerr << "Failed to receive valid file name length" << endl;
        return false;
    }

    char fileName[256] = {0};
    ret = recv(session.socket, fileName, nameLen, 0);
    if (ret <= 0) {
        cerr << "Failed to receive file name" << endl;
        return false;
    }
    fileName[nameLen] = '\0';

    uint64_t fileSize = 0;
    ret = recv(session.socket, (char*)&fileSize, sizeof(fileSize), 0);
    if (ret <= 0 || fileSize == 0) {
        cerr << "Failed to receive valid file size" << endl;
        return false;
    }

    // Try to open the file safely
    ofstream outFile(saveDir + "/" + fileName, ios::binary);
    if (!outFile) {
        cerr << "Failed to open file for writing: " << saveDir + "/" + fileName << endl;
        return false;
    }

    char buffer[4096];
    uint64_t received = 0;

    while (received < fileSize) {
        int toRead = min((uint64_t)sizeof(buffer), fileSize - received);
        int byteCount = recv(session.socket, buffer, toRead, 0);
        if (byteCount <= 0) {
            cerr << "File transfer interrupted or client disconnected" << endl;
            return false;
        }
        outFile.write(buffer, byteCount);
        received += byteCount;
    }

    cout << "File Received: " << fileName << endl;

    vector<string> logCols = {"user_id", "file_name", "file_path"};
    vector<string> logVals = {to_string(session.userId), fileName, saveDir + "/" + fileName};
    insertIntoTable(db, "files", logCols, logVals);

    vector<string> msgCols = {"user_id", "action"};
    vector<string> msgVals = {to_string(session.userId), "File Sent"};
    insertIntoTable(db, "logs", msgCols, msgVals);

    return true;
}
void handle_client(SOCKET clientSocket, sqlite3 *db)
{
    bool Connected = true;
    char cmdBuffer[200];
    clientSession session(clientSocket);
    while (Connected)
    {

        int byteCount = recv(clientSocket, cmdBuffer, sizeof(cmdBuffer)-1, 0);
        if (byteCount <= 0)
            break;
        cmdBuffer[byteCount] = '\0';

        if (session.authenticated)
        {
            if (strcmp(cmdBuffer, "shutdown") == 0)
            {
                handle_shutdown(session, Connected);
            }
            else if (strcmp(cmdBuffer, "msg") == 0)
            {
                handle_msg(session, db);
            }
            else if(strcmp(cmdBuffer, "sendfile") == 0){
                if(!(receiveFile(session, "../fileRepo", db)))continue;
            }else if(strcmp(cmdBuffer, "viewfiles") == 0){
                sendFileList(db,session);
            }
            else if(strcmp(cmdBuffer, "downloadfile") == 0){
                sendFile(session,db);
            }
        }
        else if (strcmp(cmdBuffer, "register") == 0)
        {
            handle_reg(session, db);
        }
        else if (strcmp(cmdBuffer, "login") == 0)
        {
            handle_login(session, db);
        }
        else
        {
            cout << "Please Login before sending Commands" << endl;
        }
    }

    closesocket(clientSocket);
    cout << "Client disconnected: " <<session.username<<" (ID: "<<session.userId<<")"<< endl;
    vector<string> logCols = {"user_id", "action"};
    vector<string> logVals = {to_string(session.userId), "Client Disconnect"};
    insertIntoTable(db, "logs", logCols, logVals);
}