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

#define MAXDATASIZE 528 // nombre max d'octets on peut envoyer en un coup 


void read_data(int fd, int sfd){

    /* Values needed */
    int seqnum = 0; // correspond au numéro de séquence
    char encoded_data[MAXDATASIZE]; // stocke le paquet encodé 
    int end_file = 0; // si le fichier n'est pas vide, sinon 1 

    /* Packet creation */
    pkt_t *pkt_data = pkt_new();
    if(pkt == NULL) return;


    nfds_t nfds = 2; 
    char buf1[MAXDATASIZE];
    char buf2[MAXDATASIZE];
    struct pollfd fds[2];
    int timeout = 6000; // 6 secondes; 


    while(end_file == 0){
        memset((void *)buf1 0, MAXDATASIZE);
        memset((void *)buf2, 0, MAXDATASIZE);
        memset(fds,0,nfds*sizeof(struct pollfd));
        (fds[0]).fd = sfd;
        (fds[0]).events = POLLIN|POLLOUT ;
        (fds[1]).fd = fd; 
        (fds[1]).events = POLLIN;

        int err; 
        int po = poll(fds,nfds,timeout);
        if(po == -1)
        {
            fprintf(stderr,"ERR0R in poll function : %s \n",strerror(errno));
            pkt_del(pkt);
            return -1;
        } 
        else if(po > 0) 
        {
            int length; 

            // lecture dans le fichier 
            if(fds[1].revents & POLLIN)  
            {
                length = read(fd,buf1,MAXDATASIZE); // nombre d'octets lus 
                if(length < 0)
                {
                    fprintf(stderr," ERROR in reading file : %s \n",strerror(errno));
                    pkt_del(pkt_data);
                    return;
                }
                else if(length == 0) end_file = 1; // fin du fichier 
                else
                {
                    pkt_set_payload(pkt_data,buf1,length);
                    pkt_set_seqnum(pkt_data,seqnum);

                    size_t len = MAXDATASIZE;
                    pkt_status_code status = pkt_encode(pkt_data,encode_data,&len);

                    if(status != PKT_OK)
                    {
                        fprintf(stderr," ERROR in pkt_encode \n");
                    }

                    // on envoie au receiver 
                    err = write(sfd,pkt_data,len);
                    if(err < 0)
                    {
                        fprintf(stderr,"ERROR in sending packet \n");
                        pkt_del(pkt_data);
                        return; 
                    }
                }


            }
            // on reçois un ACK / NACK 
            if(fds[0].revents & POLLIN)
            {
                length = read(fds,buf2,MAXDATASIZE);
                if(length < 0)
                {
                    
                }

                /*************
                   * TODO *
                **************/
            }
        }

    }
}

void send_packet(const char *hostname, int port,const char *file)
{

    /* Hostname convertion into sockaddr_in6 address */

    struct sockaddr_in6 address;
    memset(&address,0,sizeof(struct sockaddr_in6));
    const char* msg = real_address(hostname, &address);
    if(msg != NULL)
    {
        fprintf(stderr, "%s \n",sterror(errno));
        return; 
    }

    /* Socket creation & connection to receiver address & port */

    int sfd = create_socket(NULL, 0, &address, port);
    if(sfd == -1) return; 

    /* File (or STDIN) opening */

    if(file != NULL) int fd = open(file, O_RDONLY);
    else int fd = STDIN_FILENO;

    /* File reading -> Packet filling  */


    close(sfd);
    close(fd);

    return;
}



