# Radio Server Project

C++ server-client application using SQLite for data storage and logging. Features user registration, login, messaging, and file upload/download.

## Folder Structure

/server  
    /src  
    /include  
    /libs  
    /bin  
    /build  
/client  
    /src  
    /include  
    /bin  
    /build  
.gitignore  
README.md  

## Build Instructions

### Server
cd server  
mkdir build  
cd build  
cmake ..  
cmake --build .  

Executable will be in /server/bin  

### Client
cd client  
mkdir build  
cd build  
cmake ..  
cmake --build .  

Executable will be in /client/bin  

## Notes
- Uses C++17 for std::filesystem
- SQLite is compiled directly from libs/sqlite3.c
- .gitignore ignores build outputs and binaries (bin/ and build/ folders)
- Server and client are built separately
- run server.exe first becuase client requires a server to connect to