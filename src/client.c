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
#include <sys/stat.h>
#include <fcntl.h>
#include <poll.h>

#include "client.h"
#include "send_receive.h"
#include "packet_interface.h"

#define MAXDATASIZE 528 // nombre max d'octets on peut envoyer en un coup 


int read_write_loop(int fd, int sfd){

    /* Valurs nécessaires */
    
    int seqnum = 0; // correspond au numéro de séquence
    int end_file = 0; // si le fichier n'est pas vide, sinon 1 
    int window = 1; //valeur initiale de window
    int timeout = 5000; // 5 secondes; 

    /* Création de paquets */

    pkt_t *pkt_data = pkt_new();
    if(pkt_data == NULL){
        pkt_del(pkt_data);
        return -1; 
    }
    pkt_set_window(pkt_data,window);

    pkt_t *pkt_ack = pkt_new();
    if(pkt_ack == NULL){
        pkt_del(pkt_data);
        pkt_del(pkt_ack);
        return -1;
    }
    pkt_set_window(pkt_ack,window);


    nfds_t nfds = 2; 
    char buf1[MAXDATASIZE];
    char buf2[MAXDATASIZE];
    struct pollfd fds[2];

    /* Boucle pour envoyer et recevoir des données */

    while(end_file == 0){
        memset((void *)buf1, 0, MAXDATASIZE);
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
            pkt_del(pkt_data);
            pkt_del(pkt_ack);
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
                    pkt_del(pkt_ack);
                    return -1; 
                }
                else if(length == 0) end_file = 1; // fin du fichier 
                else
                {
                    pkt_set_payload(pkt_data,buf1,length);
                    pkt_set_seqnum(pkt_data,seqnum);

                    size_t len = MAXDATASIZE;
                    pkt_status_code status = pkt_encode(pkt_data,buf1,&len);

                    if(status != PKT_OK)
                    {
                        fprintf(stderr," ERROR in pkt_encode : %s \n", strerror(errno));
                        pkt_del(pkt_data);
                        pkt_del(pkt_ack);
                        return -1;
                    }

                    // on envoie au receiver 
                    err = write(sfd,buf1,len);
                    if(err < 0)
                    {
                        fprintf(stderr,"ERROR in sending packet : %s \n", strerror(errno));
                        pkt_del(pkt_data);
                        pkt_del(pkt_ack);
                        return -1; 
                    }

                    fprintf(stderr,"Packet %d sent \n",seqnum);

                    seqnum = (seqnum+1)%256;
                }


            }
            // on reçois un ACK / NACK 
            if(fds[0].revents & POLLIN)
            {
                length = read(sfd,buf2,MAXDATASIZE);
                if(length < 0)
                {
                    fprintf(stderr," ERROR in recieving ack : %s \n",strerror(errno));
                    pkt_del(pkt_data);
                    pkt_del(pkt_ack);
                }
                int seq =  pkt_get_seqnum(pkt_ack);
                if(length > 0 && pkt_decode(buf2,length,pkt_ack) != PKT_OK)
                {
                    fprintf(stderr,"ERROR in recieving ack num : %d \n",seq);
                }
                else
                {
                    int ww = pkt_get_window(pkt_ack);

                    if(ww > window)
                    {
                        window = ww; 
                        pkt_set_window(pkt_data,window);
                    }

                    if(pkt_get_type(pkt_ack) == PTYPE_ACK){
                        fprintf(stderr,"Ack %d recieved \n",seq);

                    }
                    else if(pkt_get_type(pkt_ack) == PTYPE_NACK){
                        fprintf(stderr, "Nack %d recieved \n",seq);

                    }


                }
            }
        }

    }

    // FIn du fichier -> on envoie un packet de taille 0 pour indiquer la fin du transfert 

    pkt_set_length(pkt_data,0);
    pkt_set_seqnum(pkt_data,seqnum);

    size_t len = MAXDATASIZE;
    pkt_status_code status = pkt_encode(pkt_data,buf1,&len);

    if(status != PKT_OK)
    {
        fprintf(stderr," ERROR in pkt_encode : %s \n", strerror(errno));
        pkt_del(pkt_data);
        pkt_del(pkt_ack);
        return -1;
    }

    // on envoie au receiver 
    int err = write(sfd,buf1,len);
    if(err < 0)
    {
        fprintf(stderr,"ERROR in sending packet : %s \n", strerror(errno));
        pkt_del(pkt_data);
        pkt_del(pkt_ack);
        return -1;
    }

    int end = 0;

    while(end == 0) //on attend de recevoir le dernier ack 
    {
        memset((void *)buf1, 0, MAXDATASIZE);
        memset((void *)buf2, 0, MAXDATASIZE);
        memset(fds,0,nfds*sizeof(struct pollfd));
        (fds[0]).fd = sfd;
        (fds[0]).events = POLLIN|POLLOUT ;
        (fds[1]).fd = fd; 
        (fds[1]).events = POLLIN;
    }

    pkt_del(pkt_data);
    pkt_del(pkt_ack);
    return 0; 
}

int send_data(const char *hostname, int port, char *file, int *fd, int *sfd){

    /* Convertion de l'adresse hostname en adresse sockaddr_in6 */

    struct sockaddr_in6 address;
    memset(&address,0,sizeof(struct sockaddr_in6));
    const char* msg = real_address(hostname, &address);
    if(msg != NULL){
        fprintf(stderr,"Error in adrress convertion : %s \n", msg);
        return -1;
    }

    /* Création du socket & connection à l'adresse de réception et au port */

    *sfd = create_socket(NULL, 0, &address, port);
    if(*sfd == -1){
        close(*sfd);
        fprintf(stderr,"Error in creating socket : %s \n", strerror(errno));
        return -1; 
    }

    /* Ouverture du fichier (ou de STDIN) */

    if(file != NULL) *fd = open(file, O_RDONLY);
    else *fd = STDIN_FILENO;

    if(*fd == -1){ 
        close(*sfd); 
        close(*fd);
        fprintf(stderr,"Error in opening file : %s \n", strerror(errno));
        return -1;
    }

    fprintf(stderr,"Connection à l'adresse : %s et au port : %d \n",hostname,port);

    return 0;
}