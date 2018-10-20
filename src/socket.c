#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

/* Resolve the resource name to an usable IPv6 address
 * @address: The name to resolve
 * @rval: Where the resulting IPv6 address descriptor should be stored
 * @return: NULL if it succeeded, or a pointer towards
 *          a string describing the error if any.
 *          (const char* means the caller cannot modify or free the return value,
 *           so do not use malloc!)
 */
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

/* Creates a socket and initialize it
 * @source_addr: if !NULL, the source address that should be bound to this socket
 * @src_port: if >0, the port on which the socket is listening
 * @dest_addr: if !NULL, the destination address to which the socket should send data
 * @dst_port: if >0, the destination port to which the socket should be connected
 * @return: a file descriptor number representing the socket,
 *         or -1 in case of error (explanation will be printed on stderr)
 */
int create_socket(struct sockaddr_in6 *source_addr,
                 int src_port,
                 struct sockaddr_in6 *dest_addr,
                 int dst_port){

	int sfd = socket(AF_INET6,SOCK_DGRAM,IPPROTO_UDP); 
	if(sfd == -1){
		fprintf(stderr," ERR0R in socket : %s \n",strerror(errno));
		return -1;
	}

	if(source_addr != NULL && src_port > 0) {
		source_addr->sin6_port = htons(src_port); 
		int bin = bind(sfd,(const struct sockaddr *) source_addr,sizeof(struct sockaddr_in6));
		if(bin == -1){
			fprintf(stderr,"ERR0R in bind : %s \n",strerror(errno));
			return -1;
		}
	}

	if(dest_addr != NULL && dst_port > 0){
		dest_addr->sin6_port = htons(dst_port);
		int con = connect(sfd,(const struct sockaddr *) dest_addr,sizeof(struct sockaddr_in6));
		if(con == -1){
			fprintf(stderr,"ERR0R in connect : %s \n",strerror(errno));
			return -1; 
		}
	}

	return sfd;
}

/* Block the caller until a message is received on sfd,
 * and connect the socket to the source addresse of the received message.
 * @sfd: a file descriptor to a bound socket but not yet connected
 * @return: 0 in case of success, -1 otherwise
 * @POST: This call is idempotent, it does not 'consume' the data of the message,
 * and could be repeated several times blocking only at the first call.
 */
int wait_for_client(int sfd){
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

