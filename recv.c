#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

#define MAX_SIZE 1024 * 1024 * 8 - 1

char buf[MAX_SIZE + 1];

static inline uint16_t swap16(uint16_t x)
{
    return ((x & 0xFF) << 8) | ((x >> 8) & 0xFF);
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

void recv_mail()
{
    const char *host_name = "pop.qq.com";   // TODO: Specify the mail server domain name
    const unsigned short port = 110;        // POP3 server port
    const char *user = "qqq"; // TODO: Specify the user
    const char *pass = "aaa";  // TODO: Specify the password
    char dest_ip[16];
    int s_fd; // socket file descriptor
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

    // TODO: Create a socket,return the file descriptor to s_fd, and establish a TCP connection to the POP3 server
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

    // TODO: Send user and password and print server response
    send_frag(s_fd, "user ", user);
    print_msg(s_fd);
    send_frag(s_fd, "pass ", pass);
    print_msg(s_fd);

    // TODO: Send STAT command and print server response
    send_frag(s_fd, NULL, "stat");
    print_msg(s_fd);

    // TODO: Send LIST command and print server response
    send_frag(s_fd, NULL, "list");
    print_msg(s_fd);

    // TODO: Retrieve the first mail and print its content
    send_frag(s_fd, "retr ", "1");
    print_msg(s_fd);
    print_msg(s_fd);

    // TODO: Send QUIT command and print server response
    send_frag(s_fd, NULL, "quit");
    print_msg(s_fd);

    close(s_fd);
}

int main(int argc, char *argv[])
{
    recv_mail();
    exit(0);
}
