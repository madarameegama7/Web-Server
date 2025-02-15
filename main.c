#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h> //windows specific header fornworking with network sockets
#include <windows.h> //windows specific header for working with windows API
#include "./client/handleclient.h"
#include "server_config.h"

#pragma comment(lib, "ws2_32.lib")  //tells the compiler to link the Winsock library (ws2_32.lib), which is necessary for the socket-related functions to work correctly on Windows.

void init_winsock() {
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
        printf("Winsock initialization failed. Error Code : %d\n", WSAGetLastError());
        exit(EXIT_FAILURE);
    }
}

int main() {
    init_winsock();  // Initialize Winsock

    SOCKET server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    int addrlen = sizeof(client_addr);

    // Create socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        printf("Socket creation failed: %d\n", WSAGetLastError());
        WSACleanup();
        exit(EXIT_FAILURE);
    }

    // Bind socket
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(SERVER_PORT);

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        printf("Bind failed: %d\n", WSAGetLastError());
        closesocket(server_fd);
        WSACleanup();
        exit(EXIT_FAILURE);
    }

    // Listen for connections
    if (listen(server_fd, MAX_CONNECTIONS) == SOCKET_ERROR) {
        printf("Listen failed: %d\n", WSAGetLastError());
        closesocket(server_fd);
        WSACleanup();
        exit(EXIT_FAILURE);
    }

    printf("Server running on port %d...\n", SERVER_PORT);

    while (1) {
        client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &addrlen);
        if (client_fd == INVALID_SOCKET) {
            printf("Client accept failed: %d\n", WSAGetLastError());
            continue;
        }
        handle_client(client_fd);
    }

    closesocket(server_fd);
    WSACleanup();
    return 0;
}
