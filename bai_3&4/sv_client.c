/*
 * Bài tập 3
 *
 * Chương trình sv_client: cho phép người dùng nhập thông tin sinh viên
 * (MSSV, họ tên, ngày sinh, điểm TB) rồi đóng gói và gửi sang sv_server.
 * Cách chạy
 *   gcc -o sv_client sv_client.c
 *   ./sv_client <địa chỉ IP> <cổng>
 *   Ví dụ: ./sv_client 127.0.0.1 9090
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

/* Cấu trúc lưu thông tin sinh viên */
typedef struct {
    char mssv[20];
    char ho_ten[100];
    char ngay_sinh[15];     
    float diem_tb;       
} SinhVien;

/* Hàm đọc một dòng từ stdin */
static void doc_dong(const char *prompt, char *buf, int buf_size)
{
    printf("%s", prompt);
    if (fgets(buf, buf_size, stdin) == NULL)
        buf[0] = '\0';
    
    int len = strlen(buf);
    if (len > 0 && buf[len - 1] == '\n')
        buf[len - 1] = '\0';
}

int main(int argc, char *argv[])
{
    if (argc != 3) {
        fprintf(stderr, "Cách dùng: %s <địa chỉ IP> <cổng>\n", argv[0]);
        fprintf(stderr, "Ví dụ    : %s 127.0.0.1 9090\n", argv[0]);
        return 1;
    }

    const char *server_ip   = argv[1];
    int         server_port = atoi(argv[2]);

    if (server_port <= 0 || server_port > 65535) {
        fprintf(stderr, "Cổng không hợp lệ: %s\n", argv[2]);
        return 1;
    }

    /* 1. Tạo socket TCP */
    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == -1) {
        perror("socket() thất bại");
        return 1;
    }

    /* 2. Điền địa chỉ server và kết nối */
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family      = AF_INET;
    server_addr.sin_port        = htons(server_port);

    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        fprintf(stderr, "Địa chỉ IP không hợp lệ: %s\n", server_ip);
        close(sock);
        return 1;
    }

    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("connect() thất bại");
        close(sock);
        return 1;
    }

    printf("=== Đã kết nối đến server %s:%d ===\n\n", server_ip, server_port);

    /* 3. Nhập thông tin sinh viên */
    SinhVien sv;
    memset(&sv, 0, sizeof(sv));

    doc_dong("Nhập MSSV       : ", sv.mssv,      sizeof(sv.mssv));
    doc_dong("Nhập Họ và tên  : ", sv.ho_ten,    sizeof(sv.ho_ten));
    doc_dong("Nhập Ngày sinh  : ", sv.ngay_sinh, sizeof(sv.ngay_sinh));

    char diem_str[20];
    doc_dong("Nhập Điểm TB    : ", diem_str, sizeof(diem_str));
    sv.diem_tb = (float)atof(diem_str);

    /* 4. Đóng gói thành chuỗi văn bản và gửi đi */
    /* Định dạng: "MSSV|HO_TEN|NGAY_SINH|DIEM_TB\n" */
    char payload[BUFFER_SIZE];
    int  payload_len = snprintf(payload, sizeof(payload),
                                "%s|%s|%s|%.2f\n",
                                sv.mssv,
                                sv.ho_ten,
                                sv.ngay_sinh,
                                sv.diem_tb);

    if (payload_len <= 0 || payload_len >= (int)sizeof(payload)) {
        fprintf(stderr, "Dữ liệu quá dài, không thể đóng gói.\n");
        close(sock);
        return 1;
    }

    int sent = send(sock, payload, payload_len, 0);
    if (sent == -1) {
        perror("send() thất bại");
        close(sock);
        return 1;
    }

    printf("\n[OK] Đã gửi %d byte đến server.\n", sent);
    printf("     Nội dung gửi: %s", payload);

    /* 5. Đóng kết nối */
    close(sock);
    printf("[OK] Đã đóng kết nối.\n");
    return 0;
}
