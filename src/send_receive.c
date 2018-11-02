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

const char * real_address(const char *address, struct sockaddr_in6 *rval){

	struct addrinfo hints;

	memset(&hints,0,sizeof(struct addrinfo));
	hints.ai_family = AF_INET6;  // IPv6
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = 0;
	hints.ai_protocol = IPPROTO_UDP; // protocol UDP
	hints.ai_canonname = NULL;
	hints.ai_addr = NULL;
	hints.ai_next = NULL;

	struct addrinfo *res;

	int val = getaddrinfo(address,NULL,&hints,&res);
	if(val != 0)
	return gai_strerror(val);
	*rval = *(struct sockaddr_in6 *) res->ai_addr;

	return NULL;
}

int create_socket(struct sockaddr_in6 *source_addr, int src_port,
	struct sockaddr_in6 *dest_addr,int dst_port){

	int sfd = socket(AF_INET6,SOCK_DGRAM,IPPROTO_UDP);
	if(sfd == -1){
		fprintf(stderr," Erreur dans la fonction socket : %s \n",strerror(errno));
		return -1;
	}

	if(source_addr != NULL && src_port > 0) {
		source_addr->sin6_port = htons(src_port);
		int bin = bind(sfd,(const struct sockaddr *) source_addr,sizeof(struct sockaddr_in6));
		if(bin == -1){
			fprintf(stderr,"Erreur dans la fonction bind : %s \n",strerror(errno));
			return -1;
		}
	}

	if(dest_addr != NULL && dst_port > 0){
		dest_addr->sin6_port = htons(dst_port);
		int con = connect(sfd,(const struct sockaddr *) dest_addr,sizeof(struct sockaddr_in6));
		if(con == -1){
			fprintf(stderr,"Erreur dans la fonction connect : %s \n",strerror(errno));
			return -1;
		}
	}

	return sfd;
}
