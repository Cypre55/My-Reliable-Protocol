#include "rsocket.h"
#include <stdlib.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <netdb.h>

#define PORT 50090
#define SEND_BUF_SIZE 51
#define HOSTNAME "127.0.0.1"
#define DEST_PORT 50091

int main(void)
{
    int sockfd;
    struct sockaddr_in my_addr;
    struct sockaddr_in their_addr;
    struct hostent *he;
    int sin_size = sizeof(struct sockaddr_in);

    // Create MRP Socket M1
    if ((he=gethostbyname(HOSTNAME)) == NULL) {
        perror("gethostbyname");
        exit(1);
    }

    sockfd = r_socket(AF_INET, SOCK_MRP);
    their_addr.sin_family = AF_INET;
    their_addr.sin_port = htons(DEST_PORT);
    their_addr.sin_addr = *((struct in_addr *)he->h_addr);
    memset(&(their_addr.sin_zero), '\0', 8);

    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(PORT);
    my_addr.sin_addr.s_addr = INADDR_ANY;
    memset(&(my_addr.sin_zero), '\0', 8);
    r_bind(sockfd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr));

    // Take long string (25 < string size < 50)
    char *send_buf = (char *) calloc(SEND_BUF_SIZE, sizeof(char));
    printf("Enter string: ");
    scanf("%s", send_buf);

    // Sendto for each character
    for (int i = 0; i < strlen(send_buf); i++)
    {
        if (r_sendto(sockfd, send_buf + i, sizeof(char), MSG_CONFIRM, (struct sockaddr *)&their_addr, sin_size) == -1)
            perror("r_send");
    }

    r_close(sockfd);

}

