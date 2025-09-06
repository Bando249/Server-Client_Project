add_executable(
    client 
    main.cpp
    clientHandles.cpp
    network.cpp
    client.rc
)

target_include_directories(client PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(client PRIVATE ws2_32)