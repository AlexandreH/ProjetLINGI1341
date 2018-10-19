#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "client.h"
#include "send_receive.h"
#include "packet_interface.h"



void send_packet(const char *hostname, int port,const char *file)
{

    /* Hostname convertion into sockaddr_in6 address */

    struct sockaddr_in6 address;
    memset(&address,0,sizeof(struct sockaddr_in6));
    const char* msg = real_address(hostname, &address);
    if(msg != NULL){
        fprintf(stderr, "%s \n",sterror(errno));
    }

    /* Socket creation & connection to receiver address & port */

    int sfd = create_socket(NULL, 0, &address, port);
    if(sfd == -1) return; 

    /* File (or STDIN) opening */

    if(file != NULL) int fd = open(file, O_RDONLY);
    else int fd = STDIN_FILENO;

    /* Packet creation */

    pkt_t *pkt = pkt_new();
    if(pkt == NULL) return;

    /* File reading -> Packet filling  */


    while(buffer)


    close(sfd);
    close(fd);

    return;
}



