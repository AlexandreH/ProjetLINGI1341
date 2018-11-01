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
#include <poll.h>
#include <sys/select.h>

#include "server.h"
#include "send_receive.h"
#include "packet_interface.h"


int wait_for_client(int sfd){

	socklen_t address_len = sizeof(struct sockaddr_in6);
	struct sockaddr_in6 client_addr;
	memset(&client_addr,0,address_len);
	char buf[MAX_DATA_SIZE]; 

	/* Attente de réception d'un message */

	int msg = recvfrom(sfd,buf,sizeof(buf),MSG_PEEK,(struct sockaddr * restrict) &client_addr,&address_len);
	if(msg == -1){
		fprintf(stderr,"Erreur dans la fonction recvfrom : %s \n", strerror(errno));
		return -1;
	}

	/* Connection du socket à l'adresse qui a envoyé un message */

	int con = connect(sfd,(const struct sockaddr *) &client_addr,address_len);
	if(con == -1){
		fprintf(stderr,"Erreur de connection d'un socket à une adresse : %s \n",strerror(errno));
		return -1; 
	}

	return 0;
}

int read_write_loop(int fd,int sfd){
    
    /* Valurs nécessaires */
    
    int seqnum_waited = 0; // correspond au numéro de séquence attendu
    int end_file = 1; // 1 tant qu'on reçoit des données, sinon 0
    int timeout = 5000; // 5 secondes; 
    int window = 5; 
    int err;
    pkt_status_code status;

    /* Création de paquets */

    /* Paquet pour recevoir des données */
    pkt_t *pkt_data = pkt_new(); 
    if(pkt_data == NULL){
        fprintf(stderr,"Erreur lors de la création d'un paquet \n");
        pkt_del(pkt_data);
        return -1; 
    }

    /* Paquet pour envoyer des acquittements */
    pkt_t *pkt_ack = pkt_new();
    if(pkt_ack == NULL){
        fprintf(stderr,"Erreur lors de la création d'un paquet \n");
        pkt_del(pkt_data);
        pkt_del(pkt_ack);
        return -1;
    }

    nfds_t nfds = 2; 
    char *payload; 
    char encoded_pkt[MAX_DATA_SIZE]; // pour encoder le paquet 
    struct pollfd fds[2];

    /* Boucle pour recevoir et envoyer des données */

    while(end_file)
    {
        memset(fds,0,nfds*sizeof(struct pollfd));
        memset((void *)encoded_pkt, 0, MAX_DATA_SIZE);
        (fds[0]).fd = sfd;
        (fds[0]).events = POLLIN|POLLOUT;
        (fds[1]).fd = fd; 
        (fds[1]).events = POLLOUT;

        int po = poll(fds,nfds,timeout);
        if(po == -1)
        {
            fprintf(stderr,"Erreur dans la fonction poll : %s \n",strerror(errno));
            pkt_del(pkt_data);
            pkt_del(pkt_ack);
            return -1;
        } 
        else if(po > 0)
        {
            int length; 

            // réception de données
            if(fds[0].revents & POLLIN)
            {
                length = read(sfd,encoded_pkt,MAX_DATA_SIZE);
                if(length < 0)
                {
                    fprintf(stderr," Erreur lors de la lecture de données : %s \n",strerror(errno));
                    pkt_del(pkt_data);
                    pkt_del(pkt_ack);
                    return -1; 
                } 
                else
                {
                    status = pkt_decode(encoded_pkt,length,pkt_data);
                    if(status == PKT_OK)
                    {
                        int seqnum = pkt_get_seqnum(pkt_data);
                        int tr = pkt_get_tr(pkt_data);
                        int len = pkt_get_length(pkt_data);
                        uint32_t timestamp = pkt_get_timestamp(pkt_data);

                        payload = malloc(sizeof(char)*len);
                        payload =(char *) pkt_get_payload(pkt_data);
                        err = write(fd,payload,len);
                        fprintf(stderr," %s \n",payload);
                        if(err == -1)
                        {
                            fprintf(stderr,"Erreur lors de l'écriture d'un payload : %s \n", strerror(errno));
                            pkt_del(pkt_data);
                            pkt_del(pkt_ack);
                            return -1; 
                        }

                        if(tr == 1)
                        {
                            // ENVOI D UN NACK
                            err = send_ack(pkt_ack,sfd,PTYPE_NACK,seqnum,timestamp,window);
                            if(err == -1){
                                fprintf(stderr,"Erreur lors de l'envoi de l'acquittement : %s \n",strerror(errno));
                            }
                        }
                        else
                        {
                            if(seqnum != seqnum_waited){
                               fprintf(stderr,"Le numéro de séquence reçu %d est différent de celui attendu %d \n", seqnum, seqnum_waited);
                            }

                            // ENVOI D UN ACK
                            err = send_ack(pkt_ack,sfd,PTYPE_ACK,seqnum,timestamp,window);
                            if(err == -1){
                                fprintf(stderr,"Erreur lors de l'envoi de l'acquittement : %s \n",strerror(errno));
                            }

                            fprintf(stderr, "Acquittement numéro %i envoyé \n",seqnum);

                            if(len == 0){
                            // fin de la réception de données 
                                end_file = 0; 
                                fprintf(stderr,"Fin de la réception de données \n");
                            }

                            seqnum_waited++;
                        }
                    }
                }
            }
        }
    }

    pkt_del(pkt_data);
    pkt_del(pkt_ack);
    return 0; 
}

int receive_data(const char* hostname, int port, char* file, int *fd, int *sfd){

	/* Convertion de l'adresse hostname en adresse sockaddr_in6 */

    struct sockaddr_in6 address;
    memset(&address,0,sizeof(struct sockaddr_in6));
    const char* msg = real_address(hostname, &address);
    if(msg != NULL){
        fprintf(stderr,"Erreur lors de la convertion d'adresse : %s \n", msg);
        return -1;
    }

    /* Création du socket & liaison à l'adresse de réception et au port */

    *sfd = create_socket(&address, port, NULL, 0);
    if(*sfd == -1){
    	close(*sfd);
        fprintf(stderr,"Erreur lors de la création d'un socket : %s \n", strerror(errno));
        return -1; 
    }

    /* Ouverture du fichier (ou de STDOUT) */

    if(file != NULL) *fd = open(file, O_WRONLY | O_CREAT, S_IRWXU);
    else *fd = STDOUT_FILENO;
    if(*fd == -1){ 
        close(*sfd); 
        close(*fd);
        fprintf(stderr,"Erreur lors de l'ouverture d'un fichier : %s \n", strerror(errno));
        return -1;
    }

    fprintf(stderr,"Liaison à l'adresse : %s et au port : %d \n",hostname,port);

    /* Attente de réception d'un message */

    int wait = wait_for_client(*sfd); 
    if(wait == -1){
    	close(*sfd);
    	close(*fd);
    	return -1; 
    }

    fprintf(stderr,"Connection au sender \n");

    return 0; 
}

int send_ack(pkt_t * pkt, int sfd, int type, int seqnum, uint32_t timestamp, int window){

    /* Remplissage de l'acquittement */ 

    pkt_status_code status;
    int err; 
    char buf[ACK_NACK_SIZE];
    size_t length = ACK_NACK_SIZE;

    /* Type */ 
    if(type == PTYPE_ACK){
        status = pkt_set_type(pkt, PTYPE_ACK);
    }  
    else{
        status = pkt_set_type(pkt,PTYPE_NACK);
    }
    if(status != PKT_OK){
        fprintf(stderr,"Erreur lors de la création de l'acquittement \n");
        return -1; 
    }

    /* Window */
    status = pkt_set_window(pkt,window);
    if(status != PKT_OK){
        fprintf(stderr,"Erreur lors de la création de l'acquittement \n");
        return -1; 
    }

    /* Seqnum */
    status = pkt_set_seqnum(pkt,seqnum);
    if(status != PKT_OK){
        fprintf(stderr,"Erreur lors de la création de l'acquittement \n");
        return -1; 
    }

    /* Timestamp */ 
    status = pkt_set_timestamp(pkt,timestamp);
    if(status != PKT_OK){
        fprintf(stderr,"Erreur lors de la création de l'acquittement \n");
        return -1; 
    }

    /* Payload */ 
    status = pkt_set_payload(pkt,NULL,0); // payload vide 
    if(status != PKT_OK){
        fprintf(stderr,"Erreur lors de la création de l'acquittement \n");
        return -1; 
    }

    /* Encodage de l'acquittement */ 

    status = pkt_encode(pkt,buf,&length);
    if(status != PKT_OK){
        fprintf(stderr,"Erreur lors de l(encodage de l'acquittement \n");
        return -1; 
    }

    err = write(sfd,buf,length);
    if(err == -1){
        fprintf(stderr,"Erreur lors de l'envoi de l'acquittement : %s \n", strerror(errno));
        return -1; 
    }

    return 0;
}
