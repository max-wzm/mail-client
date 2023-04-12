#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <getopt.h>
#include "base64_utils.h"

#define MAX_SIZE 1024 * 1024 * 8 - 1

char buf[MAX_SIZE + 1];

static inline uint16_t swap16(uint16_t x)
{
    return ((x & 0xFF) << 8) | ((x >> 8) & 0xFF);
}

void print_msg(int s_fd)
{
    int r_size;
    if ((r_size = recv(s_fd, buf, MAX_SIZE, 0)) == -1)
    {
        perror("recv");
        exit(EXIT_FAILURE);
    }
    buf[r_size] = '\0'; // Do not forget the null terminator
    printf("%s", buf);
}

void send_frag(int s_fd, const char *HEADER, const char *content)
{
    const char *CR_LF = "\r\n";
    if (HEADER != NULL)
    {
        send(s_fd, HEADER, strlen(HEADER), 0);
    }
    send(s_fd, content, strlen(content), 0);
    send(s_fd, CR_LF, strlen(CR_LF), 0);
}

char *read_file_content(const char *path)
{
    FILE *file = fopen(path, "r");
    if (file == NULL)
    {
        perror("open file");
        exit(EXIT_FAILURE);
    }
    FILE *file_base64 = fopen("tmp", "w");
    encode_file(file, file_base64);
    if (file == NULL)
    {
        perror("open file");
        exit(EXIT_FAILURE);
    }
    fclose(file);
    fclose(file_base64);
    file_base64 = fopen("tmp", "r");
    fseek(file_base64, 0, SEEK_END);
    int file_size = ftell(file_base64);
    fseek(file_base64, 0, SEEK_SET);
    char *content = (char *)malloc(file_size);
    fread(content, 1, file_size, file_base64);
    fclose(file_base64);
    remove("tmp");
    return content;
}

// receiver: mail address of the recipient
// subject: mail subject
// msg: content of mail body or path to the file containing mail body
// att_path: path to the attachment
void send_mail(const char *receiver, const char *subject, const char *msg, const char *att_path)
{
    const char *CR_LF = "\r\n";
    const char *end_msg = "\r\n.\r\n";
    const char *host_name = "smtp.qq.com";  // TODO: Specify the mail server domain name
    const unsigned short port = 25;         // SMTP server port
    const char *user = "###"; // TODO: Specify the user
    const char *pass = "qqq";  // TODO: Specify the password
    const char *from = "aaa"; // TODO: Specify the mail address of the sender
    char dest_ip[16];                       // Mail server IP address
    int s_fd;                               // socket file descriptor
    struct hostent *host;
    struct in_addr **addr_list;
    int i = 0;
    int r_size;

    // Get IP from domain name
    if ((host = gethostbyname(host_name)) == NULL)
    {
        herror("gethostbyname");
        exit(EXIT_FAILURE);
    }

    addr_list = (struct in_addr **)host->h_addr_list;
    while (addr_list[i] != NULL)
        ++i;
    strcpy(dest_ip, inet_ntoa(*addr_list[i - 1]));

    // TODO: Create a socket, return the file descriptor to s_fd, and establish a TCP connection to the mail server
    s_fd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in servaddr;
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = swap16(port);
    servaddr.sin_addr.s_addr = inet_addr(dest_ip);
    bzero(servaddr.sin_zero, 8);

    connect(s_fd, (struct sockaddr *)&servaddr, sizeof(struct sockaddr));

    // Print welcome message
    if ((r_size = recv(s_fd, buf, MAX_SIZE, 0)) == -1)
    {
        perror("recv");
        exit(EXIT_FAILURE);
    }
    buf[r_size] = '\0'; // Do not forget the null terminator
    printf("%s", buf);

    // Send EHLO command and print server response
    const char *EHLO = "EHLO qq.com"; // TODO: Enter EHLO command here
    send_frag(s_fd, NULL, EHLO);
    // TODO: Print server response to EHLO command
    print_msg(s_fd);

    // TODO: Authentication. Server response should be printed out.
    send_frag(s_fd, NULL, "AUTH LOGIN");
    print_msg(s_fd);
    char *user_base64 = encode_str(user), *pass_base64 = encode_str(pass);
    send_frag(s_fd, NULL, user_base64);
    print_msg(s_fd);
    send_frag(s_fd, NULL, pass_base64);
    print_msg(s_fd);
    free(user_base64);
    free(pass_base64);

    // TODO: Send MAIL FROM command and print server response
    const char *MAIL_FROM = "MAIL FROM:";
    char *mail_from_cmd = (char *)malloc(strlen(MAIL_FROM) + strlen(from) + 2);
    sprintf(mail_from_cmd, "%s<%s>", MAIL_FROM, from);
    send_frag(s_fd, NULL, mail_from_cmd);
    free(mail_from_cmd);
    print_msg(s_fd);

    // TODO: Send RCPT TO command and print server response
    const char *RCPT_TO = "RCPT TO:";
    char *rcpt_to_cmd = (char *)malloc(strlen(RCPT_TO) + strlen(receiver) + 2);
    sprintf(rcpt_to_cmd, "%s<%s>", RCPT_TO, receiver);
    send_frag(s_fd, NULL, rcpt_to_cmd);
    free(rcpt_to_cmd);
    print_msg(s_fd);

    // TODO: Send DATA command and print server response
    send_frag(s_fd, NULL, "DATA");
    print_msg(s_fd);

    // TODO: Send message data
    if (subject != NULL)
    {
        send_frag(s_fd, "Subject:", subject);
    }
    send_frag(s_fd, "From:", from);
    send_frag(s_fd, "To:", receiver);
    send_frag(s_fd, "Content-Type:multipart/mixed; boundary=", "200110611");
    send_frag(s_fd, NULL, CR_LF);

    if (msg)
    {
        send_frag(s_fd, NULL, "--200110611");
        send_frag(s_fd, "Content-Type:", "text/plain");
        send_frag(s_fd, NULL, CR_LF);
        send_frag(s_fd, NULL, msg);
    }

    if (att_path)
    {
        char *content = read_file_content(att_path);
        send_frag(s_fd, NULL, "--200110611");
        send_frag(s_fd, "Content-Type:", "application/octet-stream");
        send_frag(s_fd, "Content-Transfer-Encoding:", "base64");
        send_frag(s_fd, "Content-Disposition:attachment; filename=", att_path);
        send_frag(s_fd, NULL, CR_LF);
        send_frag(s_fd, NULL, content);
        free(content);
    }

    // TODO: Message ends with a single period
    send(s_fd, end_msg, strlen(end_msg), 0);
    print_msg(s_fd);

    // TODO: Send QUIT command and print server response
    send_frag(s_fd, NULL, "QUIT");
    print_msg(s_fd);

    close(s_fd);
}

int main(int argc, char *argv[])
{
    int opt;
    char *s_arg = NULL;
    char *m_arg = NULL;
    char *a_arg = NULL;
    char *recipient = NULL;
    const char *optstring = ":s:m:a:";
    while ((opt = getopt(argc, argv, optstring)) != -1)
    {
        switch (opt)
        {
        case 's':
            s_arg = optarg;
            break;
        case 'm':
            m_arg = optarg;
            break;
        case 'a':
            a_arg = optarg;
            break;
        case ':':
            fprintf(stderr, "Option %c needs an argument.\n", optopt);
            exit(EXIT_FAILURE);
        case '?':
            fprintf(stderr, "Unknown option: %c.\n", optopt);
            exit(EXIT_FAILURE);
        default:
            fprintf(stderr, "Unknown error.\n");
            exit(EXIT_FAILURE);
        }
    }

    if (optind == argc)
    {
        fprintf(stderr, "Recipient not specified.\n");
        exit(EXIT_FAILURE);
    }
    else if (optind < argc - 1)
    {
        fprintf(stderr, "Too many arguments.\n");
        exit(EXIT_FAILURE);
    }
    else
    {
        recipient = argv[optind];
        send_mail(recipient, s_arg, m_arg, a_arg);
        exit(0);
    }
}
