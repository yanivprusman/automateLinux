#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/select.h>

#define SOCKET_PATH "/tmp/testTime.sock"
#define MAX_CLIENTS 10
#define BUFFER_SIZE 256

int main() {
    int server_fd, client_fd, max_fd;
    struct sockaddr_un addr;
    fd_set read_fds, all_fds;
    int clients[MAX_CLIENTS] = {0};
    
    if ((server_fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        exit(1);
    }

    unlink(SOCKET_PATH);
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        exit(1);
    }

    if (listen(server_fd, MAX_CLIENTS) < 0) {
        perror("listen");
        exit(1);
    }

    FD_ZERO(&all_fds);
    FD_SET(server_fd, &all_fds);
    max_fd = server_fd;

    while (1) {
        read_fds = all_fds;
        if (select(max_fd + 1, &read_fds, NULL, NULL, NULL) < 0) {
            perror("select");
            exit(1);
        }

        // New client
        if (FD_ISSET(server_fd, &read_fds)) {
            if ((client_fd = accept(server_fd, NULL, NULL)) < 0) {
                perror("accept");
                continue;
            }
            FD_SET(client_fd, &all_fds);
            if (client_fd > max_fd) max_fd = client_fd;
            printf("Client connected: %d\n", client_fd);
        }

        // Existing clients
        for (int i = 0; i <= max_fd; i++) {
            if (i != server_fd && FD_ISSET(i, &read_fds)) {
                char buffer[BUFFER_SIZE];
                int n = read(i, buffer, sizeof(buffer) - 1);
                if (n <= 0) {
                    close(i);
                    FD_CLR(i, &all_fds);
                    printf("Client disconnected: %d\n", i);
                } else {
                    buffer[n] = '\0';
                    printf("Received: %s\n", buffer);
                    // send response
                    char *response = "Hello, World!\n";
                    write(i, response, strlen(response));
                }
            }
        }
    }

    close(server_fd);
    unlink(SOCKET_PATH);
    return 0;
}
