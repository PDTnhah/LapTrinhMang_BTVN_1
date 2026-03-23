/*
 * sv_server.c
 * Bài tập 4
 *
 * Chương trình sv_server: nhận dữ liệu sinh viên từ sv_client,
 * in ra màn hình và đồng thời ghi vào file log.
 *
 * Mỗi dòng log có dạng:
 *   <IP_client> <YYYY-MM-DD> <HH:MM:SS> <MSSV> <HO_TEN> <NGAY_SINH> <DIEM_TB>
 * Ví dụ:
 *   127.0.0.1 2023-04-10 09:00:00 20201234 Nguyen Van A 2002-04-10 3.99
 *
 * Cách dịch:
 *   gcc -o sv_server sv_server.c
 *
 * Cách chạy:
 *   ./sv_server <cổng> <tên file log>
 *   Ví dụ: ./sv_server 9090 sv_log.txt
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#define BUFFER_SIZE   1024
#define BACKLOG       5

/* Hàm lấy thời gian hiện tại dạng "YYYY-MM-DD HH:MM:SS" */
static void get_timestamp(char *buf, int buf_size)
{
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    strftime(buf, buf_size, "%Y-%m-%d %H:%M:%S", tm_info);
}

/* Hàm xử lý một kết nối client                                       */
/*   - Nhận toàn bộ dữ liệu                                           */
/*   - Phân tích chuỗi "MSSV|HO_TEN|NGAY_SINH|DIEM_TB"               */
/*   - In ra màn hình và ghi vào file log                             */
static void xu_ly_client(int client_sock,
                          const char *client_ip,
                          const char *log_file)
{
    char buf[BUFFER_SIZE];
    int  total = 0;

    /* Nhận dữ liệu (có thể đến trong nhiều đoạn nhỏ với TCP) */
    while (total < (int)sizeof(buf) - 1) {
        int n = recv(client_sock, buf + total, sizeof(buf) - 1 - total, 0);
        if (n <= 0)
            break;
        total += n;
        /* Kết thúc khi nhận được '\n' */
        if (buf[total - 1] == '\n')
            break;
    }

    if (total <= 0) {
        printf("[WARN] Không nhận được dữ liệu từ %s.\n", client_ip);
        return;
    }

    buf[total] = '\0';
    if (buf[total - 1] == '\n')
        buf[total - 1] = '\0';

    /* Phân tích chuỗi "MSSV|HO_TEN|NGAY_SINH|DIEM_TB" */
    /* Dùng strtok với dấu phân cách '|' */
    char mssv[20]       = "";
    char ho_ten[100]    = "";
    char ngay_sinh[15]  = "";
    char diem_str[20]   = "";

    /* Tạo bản sao để strtok không làm hỏng buf gốc */
    char tmp[BUFFER_SIZE];
    strncpy(tmp, buf, sizeof(tmp) - 1);
    tmp[sizeof(tmp) - 1] = '\0';

    char *token;
    token = strtok(tmp, "|");
    if (token) strncpy(mssv,      token, sizeof(mssv) - 1);

    token = strtok(NULL, "|");
    if (token) strncpy(ho_ten,    token, sizeof(ho_ten) - 1);

    token = strtok(NULL, "|");
    if (token) strncpy(ngay_sinh, token, sizeof(ngay_sinh) - 1);

    token = strtok(NULL, "|");
    if (token) strncpy(diem_str,  token, sizeof(diem_str) - 1);

    /* Kiểm tra đủ 4 trường */
    if (mssv[0] == '\0' || ho_ten[0] == '\0' ||
        ngay_sinh[0] == '\0' || diem_str[0] == '\0') {
        printf("[ERROR] Dữ liệu từ %s không đúng định dạng: \"%s\"\n",
               client_ip, buf);
        return;
    }

    float diem_tb = (float)atof(diem_str);

    /* Lấy timestamp hiện tại */
    char timestamp[30];
    get_timestamp(timestamp, sizeof(timestamp));

    /* Tạo dòng log theo định dạng yêu cầu:                             */
    /*   <IP> <DATE> <TIME> <MSSV> <HO_TEN> <NGAY_SINH> <DIEM_TB>      */
    char log_line[BUFFER_SIZE * 2];
    snprintf(log_line, sizeof(log_line),
             "%s %s %s %s %s %.2f",
             client_ip, timestamp, mssv, ho_ten, ngay_sinh, diem_tb);

    /* In ra màn hình */
    printf("\n--- Nhận từ client %s ---\n", client_ip);
    printf("  MSSV      : %s\n",   mssv);
    printf("  Họ tên    : %s\n",   ho_ten);
    printf("  Ngày sinh : %s\n",   ngay_sinh);
    printf("  Điểm TB   : %.2f\n", diem_tb);
    printf("  Thời gian : %s\n",   timestamp);
    printf("  Log line  : %s\n",   log_line);

    /* Ghi vào file log */
    FILE *fp = fopen(log_file, "a");
    if (fp == NULL) {
        perror("fopen() thất bại");
        return;
    }
    fprintf(fp, "%s\n", log_line);
    fclose(fp);

    printf("[OK] Đã ghi log vào file \"%s\".\n", log_file);
}

int main(int argc, char *argv[])
{
    if (argc != 3) {
        fprintf(stderr, "Cách dùng: %s <cổng> <file log>\n", argv[0]);
        fprintf(stderr, "Ví dụ    : %s 9090 sv_log.txt\n", argv[0]);
        return 1;
    }

    int   port     = atoi(argv[1]);
    const char *log_file = argv[2];

    if (port <= 0 || port > 65535) {
        fprintf(stderr, "Cổng không hợp lệ: %s\n", argv[1]);
        return 1;
    }

    /* 1. Tạo socket TCP */
    int listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listener == -1) {
        perror("socket() thất bại");
        return 1;
    }

    /* Cho phép tái sử dụng địa chỉ ngay sau khi server khởi động lại */
    int opt = 1;
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    /* 2. Gắn địa chỉ */
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family      = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port        = htons(port);

    if (bind(listener, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind() thất bại");
        close(listener);
        return 1;
    }

    /* 3. Lắng nghe kết nối */
    if (listen(listener, BACKLOG) == -1) {
        perror("listen() thất bại");
        close(listener);
        return 1;
    }

    printf("=== sv_server đang chạy trên cổng %d ===\n", port);
    printf("    File log : %s\n", log_file);
    printf("    Nhấn Ctrl+C để dừng.\n\n");

    /* 4. Vòng lặp chấp nhận kết nối */
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);

        printf("[...] Đang chờ kết nối mới...\n");

        int client_sock = accept(listener,
                                 (struct sockaddr *)&client_addr,
                                 &client_addr_len);
        if (client_sock == -1) {
            perror("accept() thất bại");
            continue;   /* Tiếp tục chờ client khác */
        }

        /* Lấy địa chỉ IP của client */
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr,
                  client_ip, sizeof(client_ip));

        printf("[OK] Client kết nối: %s:%d  (socket fd=%d)\n",
               client_ip, ntohs(client_addr.sin_port), client_sock);

        /* Xử lý yêu cầu và ghi log */
        xu_ly_client(client_sock, client_ip, log_file);

        /* Đóng kết nối với client này */
        close(client_sock);
        printf("[OK] Đã đóng kết nối với %s.\n\n", client_ip);
    }

    /* Không bao giờ tới đây trong trường hợp bình thường */
    close(listener);
    return 0;
}