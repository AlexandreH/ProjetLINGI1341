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

int read_write_loop(int fd, int sfd){

    /* Valeurs nécessaires */

    int seqnum = 0; // correspond au numéro de séquence
    int end_file = 1; // si le fichier n'est pas vide, sinon 0
    int window = 1; //valeur initiale de window
    int timeout = 5000; // 5 secondes;
    int err; // erreur lors de l'écriture dans le socket
    pkt_status_code status;

    /* Création de paquets */

    /* Paquet pour envoyer des données */
    pkt_t *pkt_data = pkt_new();
    if(pkt_data == NULL){
        fprintf(stderr,"Erreur lors de la création d'un paquet");
        pkt_del(pkt_data);
        return -1;
    }
    pkt_set_window(pkt_data,window); //taille initiale de 1

    /* Paquet pour recevoir des acquittements */
    pkt_t *pkt_ack = pkt_new();
    if(pkt_ack == NULL){
        fprintf(stderr,"Erreur lors de la création d'une paquet");
        pkt_del(pkt_data);
        pkt_del(pkt_ack);
        return -1;
    }
    pkt_set_window(pkt_ack,window); //taille initiale de 1


    nfds_t nfds = 2; //nombre de files descriptors
    char payload[MAX_PAYLOAD_SIZE]; // pour lire le payload
    char encoded_pkt[MAX_DATA_SIZE]; // pour encoder le paquet
    struct pollfd fds[2];

    /* Boucle pour envoyer et recevoir des données */

    while(end_file)
    {
        memset((void *)payload, 0, MAX_PAYLOAD_SIZE);
        memset((void *)encoded_pkt, 0, MAX_DATA_SIZE);
        memset(fds,0,nfds*sizeof(struct pollfd));
        (fds[0]).fd = sfd;
        (fds[0]).events = POLLIN|POLLOUT ;
        (fds[1]).fd = fd;
        (fds[1]).events = POLLIN;

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

            // lecture dans le fichier
            if(fds[1].revents & POLLIN)
            {
                length = read(fd,payload,MAX_PAYLOAD_SIZE); // nombre d'octets lus
                if(length < 0)
                {
                    fprintf(stderr," Erreur lors de la lecture d'un fichier : %s \n",strerror(errno));
                    pkt_del(pkt_data);
                    pkt_del(pkt_ack);
                    return -1;
                }
                else if(length == 0) end_file = 0; // fin du fichier
                else
                {
                    pkt_set_payload(pkt_data,payload,length);
                    pkt_set_seqnum(pkt_data,seqnum);

                    size_t len = MAX_DATA_SIZE;
                    status = pkt_encode(pkt_data,encoded_pkt,&len);

                    if(status != PKT_OK)
                    {
                        fprintf(stderr," Erreur d'encodage d'un paquet : %s \n", strerror(errno));
                        pkt_del(pkt_data);
                        pkt_del(pkt_ack);
                        return -1;
                    }

                    // on envoie au receiver
                    err = write(sfd,encoded_pkt,len);
                    if(err < 0)
                    {
                        fprintf(stderr,"Erreur lors de l'envoie d'un paquet : %s \n", strerror(errno));
                        pkt_del(pkt_data);
                        pkt_del(pkt_ack);
                        return -1;
                    }

                    fprintf(stderr,"\nPaquet numéro %d envoyé \n",seqnum);

                    seqnum = (seqnum+1)%256;
                }


            }
            // on reçois un ACK / NACK
            if(fds[0].revents & POLLIN)
            {
                length = read(sfd,encoded_pkt,ACK_NACK_SIZE);
                if(length < 0)
                {
                    fprintf(stderr," Erreur de lecture de l'acquittement : %s \n",strerror(errno));
                    pkt_del(pkt_data);
                    pkt_del(pkt_ack);
                }
                int seq =  pkt_get_seqnum(pkt_ack);
                status = pkt_decode(encoded_pkt,length,pkt_ack);
                if(length > 0 && status != PKT_OK)
                {

                    fprintf(stderr,"Erreur de décodage du paquet d'acquittemnt numéro : %d \n",seq);
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
                        fprintf(stderr,"\nAcquittement %d reçu \n\n",seq);

                    }
                    else if(pkt_get_type(pkt_ack) == PTYPE_NACK){
                        fprintf(stderr, "\nNon-acquittement %d reçu \n\n",seq);

                    }


                }
            }
        }

    }

    // Fin du fichier -> on envoie un packet de taille 0 pour indiquer la fin du transfert

    pkt_set_length(pkt_data,0);
    pkt_set_seqnum(pkt_data,seqnum);

    size_t len = MAX_DATA_SIZE;
    status = pkt_encode(pkt_data,encoded_pkt,&len);

    if(status != PKT_OK)
    {
        fprintf(stderr," Erreur d'encodage d'un paquet : %s \n", strerror(errno));
        pkt_del(pkt_data);
        pkt_del(pkt_ack);
        return -1;
    }

    // on envoie au receiver
    err = write(sfd,encoded_pkt,len);
    if(err == -1)
    {
        fprintf(stderr,"Erreur lors de l'envoie d'un paquet : %s \n", strerror(errno));
        pkt_del(pkt_data);
        pkt_del(pkt_ack);
        return -1;
    }

    int end = 1;

    while(end) //on attend de recevoir le dernier ack
    {
        memset((void *)encoded_pkt, 0, MAX_DATA_SIZE);
        memset(fds,0,nfds*sizeof(struct pollfd));
        (fds[0]).fd = sfd;
        (fds[0]).events = POLLIN|POLLOUT ;
        (fds[1]).fd = fd;
        (fds[1]).events = POLLIN;

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
            if(fds[0].revents & POLLIN)
            {
                int length = read(sfd,encoded_pkt,ACK_NACK_SIZE);
                status = pkt_decode(encoded_pkt,length,pkt_ack);
                if(status == PKT_OK){
                    fprintf(stderr,"Acquittement de fin de transmission reçu \n");
                    end = 0;
                }
            }
            else{
                err =write(sfd,encoded_pkt,len);
                if(err == -1)
                {
                    fprintf(stderr,"Erreur lors de l'envoi d'un paquet : %s \n", strerror(errno));
                    pkt_del(pkt_data);
                    pkt_del(pkt_ack);
                    return -1;
                }
            }
        }
    }

    pkt_del(pkt_data);
    pkt_del(pkt_ack);
    return 0;
}

int send_data(const char *hostname, int port, char *file, int *fd, int *sfd){

    /* Conversion de l'adresse hostname en adresse sockaddr_in6 */

    struct sockaddr_in6 address;
    memset(&address,0,sizeof(struct sockaddr_in6));
    const char* msg = real_address(hostname, &address);
    if(msg != NULL){
        fprintf(stderr,"Erreur lors de la conversion d'adresse : %s \n", msg);
        return -1;
    }

    /* Création du socket & connexion à l'adresse de réception et au port */

    *sfd = create_socket(NULL, 0, &address, port);
    if(*sfd == -1){
        close(*sfd);
        fprintf(stderr,"Erreur lors de la création d'un socket : %s \n", strerror(errno));
        return -1;
    }

    /* Ouverture du fichier (ou de STDIN) */

    if(file != NULL) *fd = open(file, O_RDONLY);
    else *fd = STDIN_FILENO;

    if(*fd == -1){
        close(*sfd);
        close(*fd);
        fprintf(stderr,"Erreur lors de l'ouverture d'un fichier : %s \n", strerror(errno));
        return -1;
    }

    fprintf(stderr,"Connection à l'adresse : %s et au port : %d \n",hostname,port);

    return 0;
}
