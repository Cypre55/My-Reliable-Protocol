#ifndef _RSOCKET_H
#define _RSOCKET_H

#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

// Define the type of connection
#define SOCK_MRP SOCK_DGRAM

// Define the timeout time T
#define T 2

// Define prob of dropping
#define P 0.5

// Max message length
// Maybe redundant
#define MAX_MSG_LEN 1000
#define MAX_BUF_LEN 1024

// Max table size
#define MAX_TABLE_SIZE 50

typedef struct Message_
{
    char msg[MAX_MSG_LEN];
    time_t time_sent;
    int dest_port;
}
Message;

// Global Table info
typedef struct Table_
{
    unsigned long long int filled; // bitset for finding if a particular index is empty
    Message msg_list[MAX_TABLE_SIZE];
}
Table;

// Pointers to the unacknowledged table and received table.
Table *Unack_table, *Recv_table;
int mrp_sockfd;

pthread_t thread_r, thread_s;

// Important
// Initialise them.
int my_port;
int their_port;

pthread_mutex_t mutex;
pthread_mutexattr_t attr;

// r_socket
int r_socket(int domain, int protocol); // type is fixed, SOCK_MRP

// r_bind
int r_bind(int sockfd, struct sockaddr *my_addr, int addrlen);

// r_sendto
int r_sendto(int sockfd, const void *msg, int msg_len, unsigned int flags, const struct sockaddr *to, int to_len);

// r_recvfrom
int r_recvfrom(int sockfd, void *buf, int buf_len, unsigned int flags, struct sockaddr *from, int *from_len);

// r_close
void r_close(int sockfd);

// dropMessage
int dropMessage(float p);

#endif
