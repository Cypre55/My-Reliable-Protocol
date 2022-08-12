#include "rsocket.h"
#include <stdlib.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <string.h>
#include <netdb.h>

#define PORT 50091
#define HOSTNAME "127.0.0.1"
#define DEST_PORT 50090
#define BUF_SIZE 50

int main(void)
{
    int sockfd;
    struct sockaddr_in my_addr;
    struct sockaddr_in their_addr;
    struct hostent *he;
    int sin_size = sizeof(struct sockaddr_in);

    // Create MRP Socket M2
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

    // BIND 50000 + 2 * ROLL NO Last 4 digit + 1

    printf("Received string: \n");
    while (1)
    {
        char *recv_buf = (char *) calloc(BUF_SIZE, sizeof(char)); 
        int numbytes;
        // Recvfrom
        if ((numbytes=r_recvfrom(sockfd, recv_buf, BUF_SIZE, MSG_WAITALL, (struct sockaddr *)&their_addr, &sin_size)) == -1) {
            perror("recv");
            exit(1);
        }

        // if (numbytes == 0)
        // {
        //     break;
        // }

        // Print whatever recv
        printf("%s", recv_buf);
        fflush(stdout);
    
    }

    r_close(sockfd);

}