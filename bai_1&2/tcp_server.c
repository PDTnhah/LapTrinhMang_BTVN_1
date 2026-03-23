/*
 * Bài tập 2 - tcp_server
 * Đợi kết nối ở cổng xác định bởi tham số dòng lệnh.
 * Mỗi khi có client kết nối:
 *   - Gửi nội dung file chào cho client
 *   - Nhận toàn bộ nội dung client gửi đến và ghi vào file log
 *
 * Cách chạy: ./tcp_server <cổng> <file_chào> <file_log>
 * Ví dụ:     ./tcp_server 9000 welcome.txt log.txt
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
#define BACKLOG     5

/*
 * Đọc toàn bộ nội dung file và gửi cho client.
 * Trả về 0 nếu thành công, -1 nếu lỗi.
 */
int send_welcome_file(int client_fd, const char *filename) {
    FILE *f = fopen(filename, "r");
    if (f == NULL) {
        perror("Lỗi mở file chào");
        return -1;
    }

    char buf[BUFFER_SIZE];
    int bytes_read;
    while ((bytes_read = fread(buf, 1, sizeof(buf), f)) > 0) {
        int sent = send(client_fd, buf, bytes_read, 0);
        if (sent == -1) {
            perror("Lỗi gửi file chào");
            fclose(f);
            return -1;
        }
    }

    fclose(f);
    return 0;
}

/*
 * Nhận toàn bộ dữ liệu từ client và ghi vào file log.
 * Trả về tổng số byte đã nhận.
 */
int receive_and_log(int client_fd, const char *log_filename,
                    const char *client_ip) {
    /* Mở file log ở chế độ append để không ghi đè */
    FILE *log = fopen(log_filename, "a");
    if (log == NULL) {
        perror("Lỗi mở file log");
        return -1;
    }

    /* Ghi thông tin client vào đầu mỗi phiên */
    fprintf(log, "=== Kết nối từ %s ===\n", client_ip);
    printf("[Server] Đang nhận dữ liệu từ client %s ...\n", client_ip);

    char buf[BUFFER_SIZE];
    int total = 0;
    int ret;

    /* Vòng lặp nhận dữ liệu cho đến khi client ngắt kết nối */
    while (1) {
        ret = recv(client_fd, buf, sizeof(buf) - 1, 0);
        if (ret <= 0) {
            /* ret == 0: client đóng kết nối bình thường */
            /* ret <  0: lỗi */
            if (ret < 0)
                perror("Lỗi nhận dữ liệu");
            break;
        }

        buf[ret] = '\0';
        total += ret;

        /* In ra màn hình server */
        printf("[Nhận từ %s] %s", client_ip, buf);
        fflush(stdout);

        /* Ghi vào file log */
        fputs(buf, log);
        fflush(log);
    }

    fprintf(log, "=== Kết thúc phiên, tổng %d bytes ===\n\n", total);
    fclose(log);
    return total;
}

int main(int argc, char *argv[]) {
    /* Kiểm tra tham số dòng lệnh */
    if (argc != 4) {
        printf("Cách dùng: %s <cổng> <file_chào> <file_log>\n", argv[0]);
        printf("Ví dụ: %s 9000 welcome.txt log.txt\n", argv[0]);
        return 1;
    }

    int port = atoi(argv[1]);
    const char *welcome_file = argv[2];
    const char *log_file     = argv[3];

    if (port <= 0 || port > 65535) {
        printf("Lỗi: Cổng không hợp lệ (phải từ 1 đến 65535)\n");
        return 1;
    }

    /* Kiểm tra file chào tồn tại */
    FILE *test = fopen(welcome_file, "r");
    if (test == NULL) {
        printf("Lỗi: Không tìm thấy file chào '%s'\n", welcome_file);
        printf("Hãy tạo file này trước khi chạy server.\n");
        return 1;
    }
    fclose(test);

    /* Tạo socket lắng nghe */
    int listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listener == -1) {
        perror("Lỗi tạo socket");
        return 1;
    }

    /* Cho phép tái sử dụng địa chỉ (tránh lỗi "Address already in use") */
    int opt = 1;
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    /* Khai báo địa chỉ server */
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family      = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port        = htons(port);

    /* Gắn socket với địa chỉ */
    if (bind(listener, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Lỗi bind");
        close(listener);
        return 1;
    }

    /* Chuyển sang trạng thái lắng nghe */
    if (listen(listener, BACKLOG) == -1) {
        perror("Lỗi listen");
        close(listener);
        return 1;
    }

    printf("=== TCP Server đang chạy ===\n");
    printf("Cổng     : %d\n", port);
    printf("File chào: %s\n", welcome_file);
    printf("File log : %s\n", log_file);
    printf("Đang chờ kết nối... (Ctrl+C để dừng)\n");
    printf("--------------------------------------------------\n");

    /* Vòng lặp chính: chấp nhận và xử lý từng client */
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        int client = accept(listener,
                            (struct sockaddr *)&client_addr,
                            &client_len);
        if (client == -1) {
            perror("Lỗi accept");
            continue;
        }

        /* Lấy địa chỉ IP của client */
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));
        int client_port = ntohs(client_addr.sin_port);

        printf("\n[+] Client mới kết nối: %s:%d\n", client_ip, client_port);

        /* Gửi file chào cho client */
        printf("[Server] Gửi file chào '%s'...\n", welcome_file);
        if (send_welcome_file(client, welcome_file) == -1) {
            printf("[Server] Không gửi được file chào.\n");
        } else {
            printf("[Server] Đã gửi file chào thành công.\n");
        }
        /* Báo hiệu đã gửi xong welcome (không gửi thêm nữa từ phía server)
           => client nhận được EOF, biết welcome đã kết thúc, chuyển sang nhập liệu */
        shutdown(client, SHUT_WR);

        /* Nhận dữ liệu từ client và ghi vào file log */
        int total = receive_and_log(client, log_file, client_ip);
        printf("[Server] Phiên kết thúc. Tổng nhận: %d bytes. Đã ghi vào '%s'.\n",
               total, log_file);

        /* Đóng kết nối với client */
        close(client);
        printf("[-] Đã đóng kết nối với %s:%d\n", client_ip, client_port);
        printf("--------------------------------------------------\n");
    }

    close(listener);
    return 0;
}