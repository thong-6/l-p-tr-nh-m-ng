#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <arpa/inet.h>

int check_login(char *user, char *pass)
{
    FILE *f = fopen("database.txt", "r");
    if (f == NULL)
    {
        printf("Loi: Khong tim thay file database.txt\n");
        return 0;
    }

    char line[256];
    char file_user[128], file_pass[128];

    while (fgets(line, sizeof(line), f))
    {
        if (sscanf(line, "%s %s", file_user, file_pass) == 2)
        {
            if (strcmp(user, file_user) == 0 && strcmp(pass, file_pass) == 0)
            {
                fclose(f);
                return 1;
            }
        }
    }
    fclose(f);
    return 0;
}

void *client_handler(void *arg)
{
    int client_socket = *(int *)arg;
    free(arg);

    char buffer[1024];
    char user[128] = {0}, pass[128] = {0};
    int is_logged_in = 0;

    send(client_socket, "Vui long dang nhap.\nUsername: ", 30, 0);
    int bytes = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    if (bytes > 0)
    {
        buffer[bytes] = 0;
        buffer[strcspn(buffer, "\r\n")] = 0;
        strcpy(user, buffer);
    }

    send(client_socket, "Password: ", 10, 0);
    bytes = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    if (bytes > 0)
    {
        buffer[bytes] = 0;
        buffer[strcspn(buffer, "\r\n")] = 0;
        strcpy(pass, buffer);
    }

    if (check_login(user, pass))
    {
        char *success_msg = "Dang nhap thanh cong. Nhap lenh Linux:\n> ";
        send(client_socket, success_msg, strlen(success_msg), 0);
        is_logged_in = 1;
    }
    else
    {
        char *fail_msg = "Sai user hoac pass. Ngat ket noi.\n";
        send(client_socket, fail_msg, strlen(fail_msg), 0);
        close(client_socket);
        pthread_exit(NULL);
    }

    char out_filename[64];
    sprintf(out_filename, "out_%d.txt", client_socket);

    while (is_logged_in)
    {
        memset(buffer, 0, sizeof(buffer));
        bytes = recv(client_socket, buffer, sizeof(buffer) - 1, 0);

        if (bytes <= 0)
            break;

        buffer[strcspn(buffer, "\r\n")] = 0;
        if (strlen(buffer) == 0)
            continue;
        if (strcmp(buffer, "exit") == 0)
            break;

        char sys_cmd[1024];
        sprintf(sys_cmd, "%s > %s", buffer, out_filename);

        system(sys_cmd);

        FILE *f_out = fopen(out_filename, "r");
        if (f_out)
        {
            char file_buf[1024];
            int bytes_read;
            int has_output = 0;

            while ((bytes_read = fread(file_buf, 1, sizeof(file_buf), f_out)) > 0)
            {
                send(client_socket, file_buf, bytes_read, 0);
                has_output = 1;
            }
            fclose(f_out);

            if (!has_output)
            {
                send(client_socket, "Da thuc thi nhung khong co output.\n", 35, 0);
            }
        }
        else
        {
            send(client_socket, "Loi khi doc file output.\n", 25, 0);
        }

        send(client_socket, "> ", 2, 0);
    }

    remove(out_filename);
    close(client_socket);
    printf("Client (Socket %d) da ngat ket noi.\n", client_socket);
    pthread_exit(NULL);
}

int main()
{
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(9000);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr));
    listen(server_socket, 5);

    printf("Telnet Server dang lang nghe tai cong 9000...\n");

    while (1)
    {
        int client_socket = accept(server_socket, NULL, NULL);
        if (client_socket < 0)
            continue;

        printf("Chap nhan ket noi moi, socket: %d\n", client_socket);

        int *new_sock = malloc(sizeof(int));
        *new_sock = client_socket;

        pthread_t thread_id;
        pthread_create(&thread_id, NULL, client_handler, (void *)new_sock);
        pthread_detach(thread_id);
    }

    close(server_socket);
    return 0;
}