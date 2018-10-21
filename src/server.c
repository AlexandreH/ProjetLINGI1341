#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h> 
#include <sys/stat.h>
#include <fcntl.h>
#include <netdb.h>

#include "server.h"
#include "send_receive.h"
#include "packet_interface.h"


int wait_for_client(int sfd)
{
	socklen_t address_len = sizeof(struct sockaddr_in6);
	struct sockaddr_in6 client_addr;
	memset(&client_addr,0,address_len);
	char buf[1024]; 


	int msg = recvfrom(sfd,buf,sizeof(buf),MSG_PEEK,(struct sockaddr * restrict) &client_addr,&address_len);
	if(msg == -1){
		fprintf(stderr,"ERROR in recvfrom : %s \n", strerror(errno));
		return -1;
	}

	int con = connect(sfd,(const struct sockaddr *) &client_addr,address_len);
	if(con == -1){
		fprintf(stderr,"ERR0R in connect : %s \n",strerror(errno));
		return -1; 
	}
	return 0;
}

void receive_data(const char *hostname, int port, char *file)
{
	struct sockaddr_in6 address;
    memset(&address,0,sizeof(struct sockaddr_in6));
    const char* msg = real_address(hostname, &address);
    if(msg != NULL)
    {
        fprintf(stderr, "%s \n",strerror(errno));
        return; 
    }

    /* Socket creation & connection to receiver address & port */

    int sfd = create_socket(&address, port, NULL, 0);
    if(sfd == -1) return; 

    /* File (or STDIN) opening */
    int fd;
    if(file != NULL) fd = open(file, O_WRONLY);
    else fd = STDIN_FILENO;


    close(sfd);
    close(fd);
    return;
}