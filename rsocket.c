#include "rsocket.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <endian.h>

int find_empty_slot();
void *execute_thread_R(void *arg);
void *execute_thread_S(void *arg);
int dropMessage(float p);

time_t htonll(time_t in)
{
    return htobe64(in);
}

void print_Table(Table *t)
{
    for (int i = 0; i < MAX_TABLE_SIZE; i++)
    {
        printf("%d ", i);
        if (t->filled & (1LL << i))
        {
            printf("Message: %s, Time: %ld\n", t->msg_list[i].msg, t->msg_list[i].time_sent);
        }
        else
        {
            printf("Empty\n");
        }
    }
}

void print_Buffer(void *buffer)
{
    int size = 21;
    for (int i = 0; i < size; i++)
    {
        printf("%d ", *((char *) buffer + i));
    }
    printf("\n");
    return;
}

time_t ntohll(time_t in)
{
    return be64toh(in);
}

int r_socket(int domain, int protocol)
{
    // Allocate space for the tables
    Unack_table = (Table *) calloc(1, sizeof(Table));
    Recv_table = (Table *) calloc(1, sizeof(Table));

    // Create a socket
    if ((mrp_sockfd = socket(domain, protocol, 0)) < 0)
    {
        perror("r_socket");
    }

    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK_NP);
    pthread_mutex_init(&mutex, &attr);


    return mrp_sockfd;
}

int r_bind(int sockfd, struct sockaddr *my_addr, int addrlen)
{
    int x;
    if ((x = bind(sockfd, my_addr, addrlen)) < 0)
    {
        perror("r_bind");
    }
    my_port = ntohs(((struct sockaddr_in*)my_addr)->sin_port);
    
    if (my_port%2)
        their_port = my_port - 1;
    else
        their_port = my_port + 1;

    // printf("My Port: %d\n", my_port);
    // printf("Their Port: %d\n", their_port);

    // Create threads
    
    pthread_create(&thread_r, NULL, execute_thread_R, NULL);
    pthread_create(&thread_s, NULL, execute_thread_S, NULL);

    return x;
}

int r_sendto(int sockfd, const void *msg, int msg_len, unsigned int flags, const struct sockaddr *to, int to_len)
{
    // send once and put it in the unack table.
    // S thread will periodically check if it is received, and if not, it'll resend and update sent time.
    // print_Table(Unack_table);
    char buffer[MAX_BUF_LEN];
    
    for (int i = 0; i < MAX_BUF_LEN; i++)
    {
        buffer[i] = 0;
    }

    // First 4 bytes are the index in the unack table.
    int offset = 0;
    int index = find_empty_slot();
    index = htonl(index);
    memcpy(buffer, &index, sizeof(int));
    offset += sizeof(int);

    their_port = ntohs(((struct sockaddr_in*)to)->sin_port);

    int their_port_network = htonl(their_port);
    memcpy(buffer + offset, &their_port_network, sizeof(int));
    offset += sizeof(int);

    msg_len = htonl(msg_len);
    memcpy(buffer + offset, &msg_len, sizeof(int));
    offset += sizeof(int);
    
    msg_len = ntohl(msg_len);
    memcpy(buffer + offset, msg, msg_len);
    offset += msg_len;

    time_t time_of_sending = htonll(time(NULL));
    memcpy(buffer + offset, &time_of_sending, sizeof(time_t));
    offset += sizeof(time_t);

    // struct sockaddr_in to_addr;
    // to_addr.sin_family = AF_INET;
    // to_addr.sin_port = htons(their_port);
    // inet_aton("127.0.0.1", &to_addr.sin_addr);
    // memset(&to_addr.sin_zero, '\0', 8);

    to_len = sizeof(struct sockaddr);


    // check order.
    // printf("Sending %d bytes\n", offset);
    sendto(mrp_sockfd, buffer, offset, 0, to, to_len);
    // printf("offset is %d\n", offset);
    // print_Buffer(buffer);
    // printf("Sent to the port %d\n", ntohs(( (struct sockaddr_in *)to)->sin_port));
    // printf("sockfd is %d and mrpsockfd is %d\n", sockfd, mrp_sockfd);
    // for (int i = 0; i < offset; i++)
    // {
    //     printf("%d ", buffer[i]);
    // }
    // printf("\n");

    // Add to the unack table
    // Check host and network protocol issues, htonl and ntohl
    Message message;
    message.dest_port = their_port;
    message.time_sent = ntohll(time_of_sending);
    memcpy(message.msg, msg, msg_len);
    message.msg[msg_len] = '\0';

    // printf("A\n");
    // printf("Index: %d\n", ntohl(index));
    Unack_table->msg_list[ntohl(index)] = message;
    // printf("A\n");

    usleep(0.5 * 1000000);

    return msg_len;

}

int r_recvfrom(int sockfd, void *buf, int buf_len, unsigned int flags, struct sockaddr *from, int *from_len)
{
    int found = 0;
    // their_port = ntohs(((struct sockaddr_in*)from)->sin_port);
    while (1)
    {
        // printf("Ok\n");
        // pthread_mutex_lock(&mutex);
        for (int i = 0; i < MAX_TABLE_SIZE; i++)
        {
            if (Recv_table->filled & (1LL << i))
            {
                // printf("Hey\n");
                strcpy(buf, Recv_table->msg_list[i].msg);
                found = strlen(buf);
                Recv_table->filled ^= (1LL << i);
                break;
            }
        }
        // pthread_mutex_unlock(&mutex);
        if (found)
        {
            break;
        }
        sleep(T);
    }
    return found;
}

int find_empty_slot()
{
    // printf("Filled is %lld\n", Unack_table->filled);
    // pthread_mutex_lock(&mutex);
    for (int i = 0; i < MAX_TABLE_SIZE; i++)
    {
        if ((Unack_table->filled & (1LL << i)) == 0)
        {
            // i'th slot is empty
            Unack_table->filled ^= (1LL << i);
            return i;
        }
    }
    // pthread_mutex_unlock(&mutex);
    return -1;
}

void *execute_thread_R(void *arg)
{
    // receive a message
    
    // assuming sockfd contains a meaningful value, and it is bound to some port.
    char buffer[MAX_BUF_LEN];

    struct sockaddr_in from_addr;
    from_addr.sin_family = AF_INET;
    from_addr.sin_port = htons(their_port);
    // printf("their port is %d\n", their_port);
    inet_aton("127.0.0.1", &from_addr.sin_addr);
    memset(&from_addr.sin_zero, '\0', 8);

    int from_len = sizeof(struct sockaddr);

    while (1)
    {

        // printf("A\n");
        int msg_len_received = recvfrom(mrp_sockfd, buffer, MAX_BUF_LEN, 0, (struct sockaddr *) &from_addr, &from_len);
        // printf("B\n");
        if (dropMessage(P))
        {
            continue;
        }

        // printf("Recevied Messange Len: %d\n", msg_len_received);

        // check if the message is an ACK message or a data message
        // First 4 bytes of buffer is index in the Unack_table
        // Next 4 bytes is the port of the sender of the data message
        // Next 4 bytes is the bytes of data
        // Next msg_len bytes have the message itself
        // Last 8 bytes has the time of sending
        int data_size;
        memcpy(&data_size, buffer + 2 * sizeof(int), sizeof(int));
        data_size = ntohl(data_size);
        // printf("%d is the size of message received: %s\n", data_size, buffer + 3 * sizeof(int));

        if (data_size == 0)
        {
            // ACK message
            // data len attribute is set to zero, but the message is sent to check if the ack is for proper message.

            // maintain the unack table.
            // printf("Ack message\n");
            int index = 0;
            memcpy(&index, buffer, sizeof(int));
            index = ntohl(index);
            printf("Ack %s\n", Unack_table->msg_list[index].msg);

            // ack is received for this message
            // check if the entry at index 'index' is this message only, or it is the second ack and that entry is already removed
            pthread_mutex_lock(&mutex);
            
            if ((Unack_table->filled & (1LL << index)) == 0)
            {
                // message is already removed from the unack table.
            }
            else if (strcmp(buffer + 3 * sizeof(int), Unack_table->msg_list[index].msg) == 0)
            {
                // remove it from the unack table.
                // bit flip.
                Unack_table->filled ^= (1LL << index);
            }
            else
            {
                // the spot is already filled with another message.
            }
            pthread_mutex_unlock(&mutex);

        }
        else
        {
            // Data message
            // printf("Data message\n");

            // maintain the recv table.
            // send the ack.
            // do not need to check if it already received.
            // in the received table, filled will have the count of messages received.
            memset(buffer + 2 * sizeof(int), '\0', sizeof(int));
            // set the data_size to zero, this is the ack message
            sendto(mrp_sockfd, buffer, msg_len_received, 0, (struct sockaddr *) &from_addr, from_len);


            int port;
            memcpy(&port, buffer + sizeof(int), sizeof(int));
            port = ntohl(port);
            time_t time;
            memcpy(&time, buffer + msg_len_received - sizeof(time_t), sizeof(time_t));
            time = ntohll(time);
            Message received;
            received.dest_port = port;
            received.time_sent = time;
            memcpy(received.msg, buffer + 3 * sizeof(int), data_size);
            received.msg[data_size] = '\0';

            // printf("The messaage is %s\n", received.msg);

            for (int i = 0; i < MAX_TABLE_SIZE; i++)
            {
                // find an empty index
                pthread_mutex_lock(&mutex);

                if ((Recv_table->filled & (1LL << i)) == 0)
                {
                    memcpy(&Recv_table->msg_list[i], &received, sizeof(Message));
                    Recv_table->filled ^= (1LL << i);
                    // printf("%d is empty to in the receive table\n", i);
                    break;
                }
                pthread_mutex_unlock(&mutex);
            }
        }

    }

    // print_Table(Unack_table);

    return NULL;
}

void *execute_thread_S(void *arg)
{
    struct sockaddr_in to_addr;
    to_addr.sin_family = AF_INET;
    to_addr.sin_port = htons(their_port);
    inet_aton("127.0.0.1", &to_addr.sin_addr);
    memset(&to_addr.sin_zero, '\0', 8);

    int to_len = sizeof(struct sockaddr);
    
    while (1)
    {
        sleep(T);

        time_t curr_time = time(NULL);
        for (int i = 0; i < MAX_TABLE_SIZE; i++)
        {
            pthread_mutex_lock(&mutex);
            if (Unack_table->filled & (1LL << i))
            {
                // Ack is not received for this yet.
                time_t sent_time = Unack_table->msg_list[i].time_sent;
                if (curr_time - sent_time >= 2 * T)
                {
                    // resend.
                    printf("Resent %s\n", Unack_table->msg_list[i].msg);
                    Unack_table->msg_list[i].time_sent = time(NULL);

                    char msg[MAX_BUF_LEN];

                    int index = htonl(i);
                    memcpy(msg, &index, sizeof(int));
                    int offset = sizeof(int);

                    int port = htonl(Unack_table->msg_list[i].dest_port);
                    memcpy(msg + offset, &port, sizeof(int));
                    offset += sizeof(int);

                    int msg_len = htonl(strlen(Unack_table->msg_list[i].msg));
                    memcpy(msg + offset, &msg_len, sizeof(int));
                    offset += sizeof(int);

                    strcpy(msg + offset, Unack_table->msg_list[i].msg);
                    msg_len = ntohl(msg_len);
                    offset += msg_len;

                    sent_time = htonll(Unack_table->msg_list[i].time_sent);
                    memcpy(msg + offset, &sent_time, sizeof(time_t));
                    offset += sizeof(time_t);

                    sendto(mrp_sockfd, msg, offset, 0, (struct sockaddr *) &to_addr, to_len);
                }
            }
            pthread_mutex_unlock(&mutex);
        }
    }

    return NULL;
}

int dropMessage(float p)
{
    float random = (float) rand() / (float) RAND_MAX;
    // printf("Random number generated is %f\n", random);

    return (random < p);
}

void r_close(int sockfd)
{
    pthread_join(thread_r, NULL);
    pthread_join(thread_s, NULL);

    free(Unack_table);
    free(Recv_table);

    close(sockfd);

    return;
}

