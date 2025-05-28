#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <errno.h>

void perror(const char* msg) {
    fprintf(stderr, "%s %s\n", msg, strerror(errno));
}

#define MAX_CLIENTS 100
#define BUF_SIZE    1024

int client_socks[MAX_CLIENTS];
int client_count = 0;
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

int echo_flag = 0;
int broadcast_flag = 0;

void broadcast_message(const char *msg, int except_sock) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < client_count; i++) {
        int sock = client_socks[i];
        if (sock != except_sock) {
            send(sock, msg, strlen(msg), 0);
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

void *handle_client(void *arg) {
    int client_sock = *(int*)arg;
    free(arg);

    char buf[BUF_SIZE];
    int len;
    while ((len = recv(client_sock, buf, sizeof(buf)-1, 0)) > 0) {
        buf[len] = '\0';
        printf("Recieved Message from Server %d : %s", client_sock, buf);

        if (echo_flag) {
            send(client_sock, buf, len, 0);
        }
        if (broadcast_flag) {
            broadcast_message(buf, client_sock);
        }
    }

    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < client_count; i++) {
        if (client_socks[i] == client_sock) {
            client_socks[i] = client_socks[--client_count];
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
    close(client_sock);
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("syntax : echo-server <port> [-e[-b]\n]");
	printf("sample : echo-server 1234 -e -b\n");
        exit(EXIT_FAILURE);
    }
    int port = atoi(argv[1]);
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "-e") == 0) echo_flag = 1;
        if (strcmp(argv[i], "-b") == 0) broadcast_flag = 1;
    }

    int serv_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (serv_sock < 0) { perror("socket"); exit(EXIT_FAILURE); }

    int opt = 1;
    setsockopt(serv_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in serv_addr = {};
    serv_addr.sin_family      = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port        = htons(port);

    if (bind(serv_sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("bind"); exit(EXIT_FAILURE);
    }
    if (listen(serv_sock, 10) < 0) {
        perror("listen"); exit(EXIT_FAILURE);
    }
    printf(">>> Server listening on port %d\n", port);

    while (1) {
        struct sockaddr_in cli_addr;
        socklen_t cli_len = sizeof(cli_addr);
        int *cli_sock = (int*)malloc(sizeof(int));
        *cli_sock = accept(serv_sock, (struct sockaddr*)&cli_addr, &cli_len);
        if (*cli_sock < 0) {
            perror("accept");
            free(cli_sock);
            continue;
        }

        pthread_mutex_lock(&clients_mutex);
        if (client_count < MAX_CLIENTS) {
            client_socks[client_count++] = *cli_sock;
        } else {
            fprintf(stderr, "Exceed Max Client\n");
            close(*cli_sock);
            free(cli_sock);
            pthread_mutex_unlock(&clients_mutex);
            continue;
        }
        pthread_mutex_unlock(&clients_mutex);

        pthread_t tid;
        pthread_create(&tid, NULL, handle_client, cli_sock);
        pthread_detach(tid);
    }

    close(serv_sock);
    return 0;
}

