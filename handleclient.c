#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <windows.h>
#include "handleclient.h"
#include "mime_utils.h"
#include "server_config.h"

void handle_client(SOCKET client_fd) {
    char buffer[BUFFER_SIZE], method[10], path[512], protocol[10];
    int bytes_received = recv(client_fd, buffer, BUFFER_SIZE - 1, 0);
    
    if (bytes_received <= 0) {
        closesocket(client_fd);
        return;
    }

    buffer[bytes_received] = '\0';

    sscanf(buffer, "%s %s %s", method, path, protocol);
    printf("Request: %s %s\n", method, path);

    // File serving logic
    char file_path[1024];
    snprintf(file_path, sizeof(file_path), "%s%s", WWW_ROOT, strcmp(path, "/") == 0 ? "/index.html" : path);

    FILE *file = fopen(file_path, "rb");
    if (file) {
        char response_header[BUFFER_SIZE];
        fseek(file, 0, SEEK_END);
        long file_size = ftell(file);
        rewind(file);

        snprintf(response_header, sizeof(response_header),
                 "HTTP/1.1 200 OK\r\n"
                 "Content-Type: %s\r\n"
                 "Content-Length: %ld\r\n"
                 "Connection: close\r\n\r\n",
                 get_mime_type(file_path), file_size);
        send(client_fd, response_header, strlen(response_header), 0);

        char file_buffer[BUFFER_SIZE];
        size_t bytes;
        while ((bytes = fread(file_buffer, 1, BUFFER_SIZE, file)) > 0) {
            send(client_fd, file_buffer, bytes, 0);
        }
        fclose(file);
    } else {
        char *not_found = "HTTP/1.1 404 Not Found\r\n"
                          "Content-Type: text/html\r\n"
                          "Connection: close\r\n\r\n"
                          "<h1>404 Not Found</h1>";
        send(client_fd, not_found, strlen(not_found), 0);
    }

    closesocket(client_fd);
}
