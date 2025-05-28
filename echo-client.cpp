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

#define BUF_SIZE 1024

int sockfd;

void *recv_thread(void *arg) {
    char buf[BUF_SIZE];
    int len;
    while ((len = recv(sockfd, buf, sizeof(buf)-1, 0)) > 0) {
        buf[len] = '\0';
        printf("%s", buf);
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <ip> <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    const char *ip = argv[1];
    int port = atoi(argv[2]);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) { perror("socket"); exit(EXIT_FAILURE); }

    struct sockaddr_in serv_addr = {};
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port   = htons(port);
    if (inet_pton(AF_INET, ip, &serv_addr.sin_addr) <= 0) {
        perror("inet_pton"); exit(EXIT_FAILURE);
    }

    if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("connect"); exit(EXIT_FAILURE);
    }

    pthread_t tid;
    pthread_create(&tid, NULL, recv_thread, NULL);

    char buf[BUF_SIZE];
    while (fgets(buf, sizeof(buf), stdin) != NULL) {
        send(sockfd, buf, strlen(buf), 0);
    }

    close(sockfd);
    return 0;
}

