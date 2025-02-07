#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define SERVER_IP "127.0.0.1" // Server IP address
#define SERVER_PORT 8080      // Server port
#define MAX_MSG_LENGTH 1024   // Maximum message length

int sock; // Global socket descriptor
pthread_t recv_thread; // Global thread for receiving messages
int running = 1; // Shared flag to control the running state

// Function to receive messages from the server
void *receive_messages(void *arg) {
    char buffer[MAX_MSG_LENGTH];
    ssize_t bytes_received;

    while (running) {
        bytes_received = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (bytes_received > 0) {
            buffer[bytes_received] = '\0';  // Null-terminate the message
            printf("Received: %s\n", buffer);
        } else if (bytes_received == 0) {
            printf("Server closed the connection.\n");
            break;
        } else {
            perror("Error receiving message");
            break;
        }
    }

    return NULL;
}

// Function to send messages to the server
void send_message() {
    char message[MAX_MSG_LENGTH];

    while (running) {
        printf("Enter message: ");
        fgets(message, sizeof(message), stdin);
        message[strcspn(message, "\n")] = '\0'; // Remove newline character

        // Handle special commands
        if (strcmp(message, "/exit") == 0) {
            printf("Exiting...\n");
            running = 0;  // Signal the receiving thread to stop
            shutdown(sock, SHUT_RDWR); // Properly shut down socket
            break;
        } else if (strcmp(message, "/help") == 0) {
            printf("Available commands:\n");
            printf("/exit - Disconnect from the server and exit the client.\n");
            printf("/help - Display this help message.\n");
            continue;
        }

        // Handle invalid commands
        if (message[0] == '/') {
            printf("Invalid command. Type '/help' for a list of valid commands.\n");
            continue;
        }

        // Send message
        if (send(sock, message, strlen(message), 0) < 0) {
            perror("Message send failed");
            break;
        }
    }

    printf("Client exiting send_message()...\n");
}

int main() {
    struct sockaddr_in server_address;

    // Create socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }
    printf("Socket created successfully.\n");

    // Configure server address
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(SERVER_PORT);

    if (inet_pton(AF_INET, SERVER_IP, &server_address.sin_addr) <= 0) {
        perror("Invalid address or address not supported");
        close(sock);
        exit(EXIT_FAILURE);
    }

    // Attempt to connect to the server
    if (connect(sock, (struct sockaddr*)&server_address, sizeof(server_address)) < 0) {
        perror("Connection to server failed");
        close(sock);
        exit(EXIT_FAILURE);
    }

    printf("Connected to the server successfully.\n");

    // Start receiving messages in a separate thread
    if (pthread_create(&recv_thread, NULL, receive_messages, NULL) != 0) {
        perror("Error creating receiving thread");
        close(sock);
        exit(EXIT_FAILURE);
    }

    // Call send_message without passing the socket
    send_message();

    // Wait for the receiving thread to finish before terminating the client
    pthread_join(recv_thread, NULL);

    // Close socket after threads are done
    close(sock);
    printf("Client fully exited.\n");

    return 0;
}
