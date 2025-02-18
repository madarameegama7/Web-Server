#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>//networking libraries required for socket programming in windows
#include <windows.h>
#include "handleclient.h"
#include "../mime_utils/mime_utils.h"
#include "../server_config.h"

void handle_client(SOCKET client_fd) {
    char buffer[BUFFER_SIZE];
    char method[10];
    char path[512];
    char protocol[10];

    //recv function reads data from the socket and stores it in the buffer
    int bytes_received = recv(client_fd, buffer, BUFFER_SIZE - 1, 0);
    
    if (bytes_received <= 0) {
        closesocket(client_fd);
        return;
    }

    
    //null terminator
    buffer[bytes_received] = '\0';

    //extract first line of request into variables
    sscanf(buffer, "%s %s %s", method, path, protocol);
    printf("Request: %s %s\n", method, path);

    // File serving logic
    char file_path[1024];
    const char *relative_path = (path[0] == '/') ? path + 1 : path;
    snprintf(file_path, sizeof(file_path), "%s/%s", WWW_ROOT, *relative_path ? relative_path : "index.html");


    FILE *file = fopen(file_path, "rb");//Open file in binary mode
    if (file) {
        //preparing response header
        char response_header[BUFFER_SIZE];
        fseek(file, 0, SEEK_END);
        //ftell()-gets the file size
        long file_size = ftell(file);
        rewind(file);

        //snprintf- instead of printing to the console, it writes the formatted data to a character buffer(string)
        //Here snprintf builds the HTTP response header safely into the header buffer, ensures the output doesn't exceed BUFFER_SIZE, preventing buffer overflow, %s is replaced by file type
        snprintf(response_header, sizeof(response_header),
                 "HTTP/1.1 200 OK\r\n"
                 "Content-Type: %s\r\n"
                 "Content-Length: %ld\r\n"
                 "Connection: close\r\n\r\n",
                 get_mime_type(file_path), file_size);

        //sending the response header to client           
        send(client_fd, response_header, strlen(response_header), 0);

        //sending file content
        char file_buffer[BUFFER_SIZE];
        size_t bytes;
        while ((bytes = fread(file_buffer, 1, BUFFER_SIZE, file)) > 0) {
            send(client_fd, file_buffer, bytes, 0);
        }
        fclose(file);
    } else {
        // File not found, serve 404.html

        //variable to hold the path to the 404.html file
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

    closesocket(client_fd);
}
}
