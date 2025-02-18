#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <windows.h>
#include "handleclient.h"
#include "../mime_utils/mime_utils.h"
#include "../server_config.h"

// Allowed MIME types
const char *allowed_mime_types[] = {
    "text/html", "text/css", "application/javascript",
    "image/png", "image/jpeg", "image/gif",
    "application/pdf", "text/plain","video/mp4"
};

int is_allowed_mime_type(const char *mime_type) {
    for (size_t i = 0; i < sizeof(allowed_mime_types) / sizeof(allowed_mime_types[0]); i++) {
        if (strcmp(mime_type, allowed_mime_types[i]) == 0) {
            return 1;  // Allowed
        }
    }
    return 0;  // Not allowed
}

void handle_client(SOCKET client_fd) {
    char buffer[BUFFER_SIZE];
    char method[10];
    char path[512];
    char protocol[10];

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
    const char *relative_path = (path[0] == '/') ? path + 1 : path;
    snprintf(file_path, sizeof(file_path), "%s/%s", WWW_ROOT, *relative_path ? relative_path : "index.html");

    // Get MIME type
    const char *mime_type = get_mime_type(file_path);

    // Check if the file type is allowed
    if (!is_allowed_mime_type(mime_type)) {
        printf("400 Bad Request: Invalid file type requested.\n");

        // Serve 400.html page
        char bad_request_path[1024];
        snprintf(bad_request_path, sizeof(bad_request_path), "%s/400.html", WWW_ROOT);
        
        FILE *bad_request_file = fopen(bad_request_path, "rb");
        if (bad_request_file) {
            fseek(bad_request_file, 0, SEEK_END);
            long file_size = ftell(bad_request_file);
            rewind(bad_request_file);

            char response_header[BUFFER_SIZE];
            snprintf(response_header, sizeof(response_header),
                     "HTTP/1.1 400 Bad Request\r\n"
                     "Content-Type: text/html\r\n"
                     "Content-Length: %ld\r\n"
                     "Connection: close\r\n\r\n",
                     file_size);
            send(client_fd, response_header, strlen(response_header), 0);

            char file_buffer[BUFFER_SIZE];
            size_t bytes;
            while ((bytes = fread(file_buffer, 1, BUFFER_SIZE, bad_request_file)) > 0) {
                send(client_fd, file_buffer, bytes, 0);
            }
            fclose(bad_request_file);
        } else {
            // If 400.html is missing, send a simple text response
            char *fallback_bad_request = "HTTP/1.1 400 Bad Request\r\n"
                                         "Content-Type: text/html\r\n"
                                         "Connection: close\r\n\r\n"
                                         "<h1>400 Bad Request</h1><p>Invalid file type requested.</p>";
            send(client_fd, fallback_bad_request, strlen(fallback_bad_request), 0);
        }

        closesocket(client_fd);
        return;
    }

    // Open requested file
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
                 mime_type, file_size);
        send(client_fd, response_header, strlen(response_header), 0);

        char file_buffer[BUFFER_SIZE];
        size_t bytes;
        while ((bytes = fread(file_buffer, 1, BUFFER_SIZE, file)) > 0) {
            send(client_fd, file_buffer, bytes, 0);
        }
        fclose(file);
    } else {
        // Serve 404.html page
        char not_found_path[1024];
        snprintf(not_found_path, sizeof(not_found_path), "%s/404.html", WWW_ROOT);
        
        FILE *not_found_file = fopen(not_found_path, "rb");
        if (not_found_file) {
            fseek(not_found_file, 0, SEEK_END);
            long file_size = ftell(not_found_file);
            rewind(not_found_file);

            char response_header[BUFFER_SIZE];
            snprintf(response_header, sizeof(response_header),
                     "HTTP/1.1 404 Not Found\r\n"
                     "Content-Type: text/html\r\n"
                     "Content-Length: %ld\r\n"
                     "Connection: close\r\n\r\n",
                     file_size);
            send(client_fd, response_header, strlen(response_header), 0);

            char file_buffer[BUFFER_SIZE];
            size_t bytes;
            while ((bytes = fread(file_buffer, 1, BUFFER_SIZE, not_found_file)) > 0) {
                send(client_fd, file_buffer, bytes, 0);
            }
            fclose(not_found_file);
        } else {
            // If 404.html is missing, send a simple text response
            char *fallback_not_found = "HTTP/1.1 404 Not Found\r\n"
                                       "Content-Type: text/html\r\n"
                                       "Connection: close\r\n\r\n"
                                       "<h1>404 Not Found</h1>";
            send(client_fd, fallback_not_found, strlen(fallback_not_found), 0);
        }
    }

    closesocket(client_fd);
}
