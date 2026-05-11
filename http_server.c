#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <arpa/inet.h>

void *http_handler(void *arg)
{
    int client = *(int *)arg;
    free(arg);

    char buf[256];
    int ret = recv(client, buf, sizeof(buf) - 1, 0);

    if (ret > 0)
    {
        buf[ret] = 0;
        puts(buf);

        char *msg = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\nXin chao cac ban\n";
        send(client, msg, strlen(msg), 0);
    }

    close(client);
    pthread_exit(NULL);
}

int main()
{
    int listener = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(3333);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    bind(listener, (struct sockaddr *)&server_addr, sizeof(server_addr));
    listen(listener, 10);

    while (1)
    {
        int client = accept(listener, NULL, NULL);
        if (client < 0)
            continue;

        printf("New client connected: %d\n", client);

        int *new_sock = malloc(sizeof(int));
        *new_sock = client;

        pthread_t thread_id;
        pthread_create(&thread_id, NULL, http_handler, (void *)new_sock);
        pthread_detach(thread_id);
    }

    close(listener);
    return 0;
}