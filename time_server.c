#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <time.h>

void *time_handler(void *arg)
{
    int client = *(int *)arg;
    free(arg);

    char buf[256];
    char res[256];

    while (1)
    {
        memset(buf, 0, sizeof(buf));
        int ret = recv(client, buf, sizeof(buf) - 1, 0);
        if (ret <= 0)
            break;

        buf[strcspn(buf, "\r\n")] = 0;
        if (strlen(buf) == 0)
            continue;

        char cmd[32] = {0}, fmt[32] = {0};
        int parsed = sscanf(buf, "%s %s", cmd, fmt);

        if (parsed != 2 || strcmp(cmd, "GET_TIME") != 0)
        {
            char *err = "Sai lenh. Vui long dung: GET_TIME [format]\n";
            send(client, err, strlen(err), 0);
            continue;
        }

        time_t t = time(NULL);
        struct tm *tm = localtime(&t);
        memset(res, 0, sizeof(res));

        if (strcmp(fmt, "dd/mm/yy") == 0)
        {
            strftime(res, sizeof(res), "%d/%m/%Y\n", tm);
        }
        else if (strcmp(fmt, "dd/mm/y") == 0)
        {
            strftime(res, sizeof(res), "%d/%m/%y\n", tm);
        }
        else if (strcmp(fmt, "mm/dd/yyy") == 0)
        {
            strftime(res, sizeof(res), "%m/%d/%Y\n", tm);
        }
        else if (strcmp(fmt, "mm/dd/yy") == 0)
        {
            strftime(res, sizeof(res), "%m/%d/%y\n", tm);
        }
        else
        {
            char *err_fmt = "Sai format. Cac format ho tro: dd/mm/yy, dd/mm/y, mm/dd/yyy, mm/dd/yy\n";
            send(client, err_fmt, strlen(err_fmt), 0);
            continue;
        }

        send(client, res, strlen(res), 0);
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
    server_addr.sin_port = htons(7000);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    bind(listener, (struct sockaddr *)&server_addr, sizeof(server_addr));
    listen(listener, 10);

    printf("Time Server dang lang nghe tren cong 7000...\n");

    while (1)
    {
        int client = accept(listener, NULL, NULL);
        if (client < 0)
            continue;

        int *new_sock = malloc(sizeof(int));
        *new_sock = client;

        pthread_t thread_id;
        pthread_create(&thread_id, NULL, time_handler, (void *)new_sock);
        pthread_detach(thread_id);
    }

    close(listener);
    return 0;
}