#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>
#include <windows.h>
#include "./client/handleclient.h"

DWORD WINAPI handle_client_thread(LPVOID client_socket) {
    SOCKET client_fd = *(SOCKET*)client_socket;
    handle_client(client_fd);
    closesocket(client_fd);
    free(client_socket);
    return 0;
}

int main() {
    WSADATA wsa;
    SOCKET server_fd, *client_fd;
    struct sockaddr_in server, client;
    int client_size = sizeof(struct sockaddr_in);

    WSAStartup(MAKEWORD(2,2), &wsa);
    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(8080);

    bind(server_fd, (struct sockaddr*)&server, sizeof(server));
    listen(server_fd, 5);
    
    printf("Server running on port 8080...\n");

    while (1) {
        client_fd = malloc(sizeof(SOCKET));
        *client_fd = accept(server_fd, (struct sockaddr*)&client, &client_size);
        if (*client_fd == INVALID_SOCKET) {
            free(client_fd);
            continue;
        }

        HANDLE thread = CreateThread(NULL, 0, handle_client_thread, client_fd, 0, NULL);
        CloseHandle(thread); // Free thread resource
    }

    closesocket(server_fd);
    WSACleanup();
    return 0;
}
