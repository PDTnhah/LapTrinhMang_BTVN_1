/*
 * Bài tập 1 - tcp_client
 * Kết nối đến server theo địa chỉ IP và cổng từ tham số dòng lệnh.
 * Nhận dữ liệu từ bàn phím và gửi đến server.
 * Cách chạy: ./tcp_client <địa_chỉ_IP> <cổng>
 * Ví dụ:     ./tcp_client 127.0.0.1 9000
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#define BUFFER_SIZE 1024

int main(int argc, char *argv[]) {
    /* Kiểm tra tham số dòng lệnh */
    if (argc != 3) {
        printf("Cách dùng: %s <địa_chỉ_IP> <cổng>\n", argv[0]);
        printf("Ví dụ: %s 127.0.0.1 9000\n", argv[0]);
        return 1;
    }

    const char *server_ip = argv[1];
    int port = atoi(argv[2]);

    if (port <= 0 || port > 65535) {
        printf("Lỗi: Cổng không hợp lệ (phải từ 1 đến 65535)\n");
        return 1;
    }

    /* Tạo socket TCP */
    int client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (client == -1) {
        perror("Lỗi tạo socket");
        return 1;
    }
    printf("Socket đã tạo thành công.\n");

    /* Khai báo địa chỉ server */
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family      = AF_INET;
    server_addr.sin_port        = htons(port);

    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        printf("Lỗi: Địa chỉ IP không hợp lệ: %s\n", server_ip);
        close(client);
        return 1;
    }

    /* Kết nối đến server */
    printf("Đang kết nối đến %s:%d ...\n", server_ip, port);
    if (connect(client, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Lỗi kết nối đến server");
        close(client);
        return 1;
    }
    printf("Kết nối thành công!\n");

    /* Nhận file chào từ server (nhận đến khi server shutdown SHUT_WR => EOF) */
    printf("--------------------------------------------------\n");
    printf("[Thông báo từ server]:\n");
    char buf[BUFFER_SIZE];
    int ret;
    while ((ret = recv(client, buf, sizeof(buf) - 1, 0)) > 0) {
        buf[ret] = '\0';
        printf("%s", buf);
    }
    printf("\n--------------------------------------------------\n");
    printf("Nhập dữ liệu để gửi đến server. Gõ 'exit' để thoát.\n");
    printf("--------------------------------------------------\n");
    while (1) {
        printf("Nhập: ");
        fflush(stdout);

        /* Đọc dữ liệu từ bàn phím */
        if (fgets(buf, sizeof(buf), stdin) == NULL) {
            printf("\nKết thúc nhập liệu (EOF).\n");
            break;
        }

        /* Kiểm tra lệnh thoát */
        if (strncmp(buf, "exit", 4) == 0) {
            printf("Đang thoát...\n");
            break;
        }

        /* Gửi dữ liệu đến server */
        int len = strlen(buf);
        int sent = send(client, buf, len, 0);
        if (sent == -1) {
            perror("Lỗi gửi dữ liệu");
            break;
        }
        printf("[Đã gửi %d bytes]\n", sent);
    }

    /* Đóng socket */
    close(client);
    printf("Đã đóng kết nối.\n");
    return 0;
}