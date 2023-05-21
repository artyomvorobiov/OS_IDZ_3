#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <time.h>


#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024
const char* proposals[] = {
        "Cafe", "Restaurant", "Home", "Lecture", "Sports", "Archery", "Countryside", "Town", "Cinema",
        "Museum", "Theatre",
        "Aircraft", "Melbourne", "Berlin", "Paris", "Masterclass", "Park", "Concert"
};
const char *names[] = {"Wade", "Dave", "Seth", "Ivan", "Riley", "Gilbert", "Jorge", "Dan", "Brian", "Roberto", "Ramon",
                       "Miles", "Liam", "Nathaniel", "Ethan", "Lewis", "Milton", "Claude"};

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <ip_address> <port>\n", argv[0]);
        return 1;
    }

    const char *ip = argv[1];
    int port = atoi(argv[2]);

    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("Socket creation failed");
        return 1;
    }

    struct sockaddr_in server_address;
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    if (inet_pton(AF_INET, ip, &(server_address.sin_addr)) <= 0) {
        perror("Invalid address/Address not supported");
        return 1;
    }

    if (bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) == -1) {
        perror("Binding failed");
        return 1;
    }

    if (listen(server_socket, MAX_CLIENTS) == -1) {
        perror("Listening failed");
        return 1;
    }

    printf("Server started. Waiting for connections...\n");

    int client_sockets[MAX_CLIENTS] = {0};
    fd_set read_fds;
    int max_socket = server_socket;


    int chosen_fan = -1; // Индекс выбранного поклонника
    int exit_requested = 0;
    while (!exit_requested) {
        FD_ZERO(&read_fds);
        FD_SET(server_socket, &read_fds);

        for (int i = 0; i < MAX_CLIENTS; ++i) {
            int client_socket = client_sockets[i];
            if (client_socket > 0)
                FD_SET(client_socket, &read_fds);

            if (client_socket > max_socket)
                max_socket = client_socket;
        }

        int activity = select(max_socket + 1, &read_fds, NULL, NULL, NULL);
        if ((activity < 0) && (errno != EINTR)) {
            perror("Select error");
            return 1;
        }

        if (FD_ISSET(server_socket, &read_fds)) {
            struct sockaddr_in client_address;
            socklen_t address_len = sizeof(client_address);

            int new_socket = accept(server_socket, (struct sockaddr *)&client_address, &address_len);
            if (new_socket == -1) {
                perror("Accepting connection failed");
                return 1;
            }

            for (int i = 0; i < MAX_CLIENTS; ++i) {
                if (client_sockets[i] == 0) {
                    client_sockets[i] = new_socket;
                    printf("New connection, socket fd: IP: %s, port: %d\n",
                           inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));
                    break;
                }
            }
        }

        for (int i = 0; i < MAX_CLIENTS; ++i) {
            int client_socket = client_sockets[i];

            if (FD_ISSET(client_socket, &read_fds)) {
                char buffer[BUFFER_SIZE];
                int read_bytes = read(client_socket, buffer, BUFFER_SIZE - 1);
                if (read_bytes <= 0) {
                    struct sockaddr_in client_address;
                    socklen_t address_len = sizeof(client_address);
                    getpeername(client_socket, (struct sockaddr *)&client_address, &address_len);

                    printf("Client disconnected, socket fd: IP: %s, port: %d\n",
                           inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));

                    close(client_socket);
                    client_sockets[i] = 0;
                } else {
                    // Обработка полученных данных от клиента
                    printf("Received message from client\n");

                    int num_fans;
                    sscanf(buffer, "%d", &num_fans);

                    if (num_fans == 0) {
                        printf("No invitations today :( .\n");
                    } else {
                        if (chosen_fan == -1) {
                            //  выбираем случайным образом
                            srand(time(NULL));
                            chosen_fan = rand() % num_fans;
                            printf("Chosen fan: %s who invites to %s\n",names[chosen_fan], proposals[chosen_fan]);
                        }
                    }

                    // Отправка ответа
                    write(client_socket, "Choice has been made!", 21);
                    exit_requested = 1;
                    break;
                }
            }
        }
    }

    close(server_socket);
    return 0;
}
