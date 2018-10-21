#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h> 
#include <sys/stat.h>
#include <fcntl.h>
#include <netdb.h>

#include "server.h"
#include "send_receive.h"
#include "packet_interface.h"


char *buffer_payload[MAX_PAYLOAD_SIZE]; //Stocke payloads recus
int buffer_len[MAX_WINDOW_SIZE]; //Stocke les tailles des payloads recus


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

void receive_data(const char* hostname, int port, char* file){


	struct sockaddr_in6 real_addr;
	memset(&real_addr, 0, sizeof(real_addr));
	
	const char* test = real_address(hostname, &real_addr);//On transforme hostname en une real_address sockaddr_in6
	if (test != NULL){
		fprintf(stderr, "Probleme avec hostname '%s'.", hostname);
		return;
	}
	
	int sfd = create_socket(&real_addr, port, NULL, 0); //Création d'un socket avec l'adresse et le port fournis par l'utilisateur dans receiver.c
	if(sfd < 0){return;}
	
	int wait = wait_for_client(sfd);//Attente de connexion
	if(wait < 0){return;}

	int fd;
	if(file != NULL)//si l'utilisateur a fournit un fichier dans lequel il veut stocker les données, on l'ouvre maintenant
	{
		fd = open((const char *)file, O_WRONLY | O_CREAT | O_TRUNC , 00700);
		}
	else//sinon, c'est envoyé sur la sortie standard.
	{
		fd = STDOUT_FILENO;
	}

	
	struct timeval tv;//structure représentant le transmission timer
	
	memset(buffer_payload,0,MAX_WINDOW_SIZE);//on met les tailles de tous les buffers à 0 (tableau de int)
	
	char bufsfd[528]; //512 de payload, 4 de header, 4 de timestamp, 4 pour CRC1 et 4 pour crc32


	//Permet de lire les packet recu
	pkt_t* pkt_recu;
	pkt_recu = pkt_new();
	if(pkt_recu == NULL){
		fprintf(stderr, "Creation de pkt impossible");
		pkt_del(pkt_recu);
		return;
	}
	
	//Permet d'envoyer des ACK/NACK 
	pkt_t* pkt_ack;
	pkt_ack = pkt_new();
	if(pkt_ack == NULL){
		fprintf(stderr, "Creation de pkt impossible");
		pkt_del(pkt_ack);
		pkt_del(pkt_recu);
		return;
	}
	
	//On continue à recevoir des messages dans que le sender n'a pas envoyé son packet indiquant la fin du transfert
	int endOfFile = 1; //tant que c'est à 0, on recommence la boucle////////////////////////////////////////////////////
	
	fd_set readfds; //set de fds
	int nfds = 0;//taille max des fd
	
	int numSeqLogique = 0;//numero de Sequence attendu pour arrivé
	int indexBuf = 0;//index que va prendre le packet dans le buffer (attention quand SeqNum >255 )
	int numeroFenetre=0;
	
	while(endOfFile){
		
		numSeqLogique++; //on s'attend à ce que le numero de Seq qui arrive vaut 1 en plus que le precedent
		FD_ZERO(&readfds); //clear the set
		FD_SET(sfd, &readfds);// on ajoute sfd au set readfds
		
		if(fd>sfd){nfds = fd+1;} // nfds should be set to the highest-numbered file descriptor in any of the three sets, plus 1.
		else {nfds = sfd+1;}
		
		//remettre les valeurs, de timeval à chaque appel de sele
		tv.tv_sec = 5;
		tv.tv_usec = 0;
		
		
		select(nfds, &readfds,NULL, NULL, &tv); //allow a program to monitor multiple file descriptors

		if(FD_ISSET(sfd, &readfds))//sfd est bien dans readfds
		 {
			
			int rd = read(sfd,(void *)bufsfd, 528);//on met le paquet recu dans le buffer
			if(rd == 0){//fin de transmission si fin du fichier
			}

			else if(rd < 0){//Probleme avec la lecture du packet
				pkt_del(pkt_recu);//suppression de la structure pkt_recu
				pkt_del(pkt_ack);//suppression de la structure pkt_ack
				printf("Taille packet recu negative\n");
				return;
			}

			else if(rd > 0){//lecture du sfd ok
				
				//Décodage du packet
				//On met le contenu du bufsfd dans pkt_recu, si pas d'erreur dans decode et que c'est un type DATA
				if((pkt_decode((const char*)bufsfd,(int)rd,pkt_recu) == PKT_OK) && (pkt_get_type(pkt_recu) == PTYPE_DATA)) 
				{
					int numSeq = pkt_get_seqnum(pkt_recu);
					int tr = pkt_get_tr(pkt_recu);
					int length = pkt_get_length(pkt_recu);
					uint32_t timestamp = pkt_get_timestamp(pkt_recu);

					
					if(tr==1)//Si tr vaut 1
					{
					int nacksend =send_ack(pkt_ack,numSeq,PTYPE_NACK,timestamp);
						if(nacksend == -1)
						{
							fprintf(stderr,"Probleme dans l'envoie du NACK");
						}
					}
					else{//tr vaut 0
						if(length == 0)
						{
							printf("FIN: packet de taille 0\n");
							endOfFile = 0 ;
						}
						
						else  //Ajout du packet recu!!
						
						{
							if(numSeq != numSeqLogique){printf("le numero de sequence recu n est pas celui attendu");}
							if(numSeq >= 255){numeroFenetre++;}
							
							indexBuf = (numeroFenetre*256)-1+numSeq;
							
							int acksend=send_ack(pkt_ack,numSeq, PTYPE_ACK,timestamp);
							if(acksend == -1)
						{
							fprintf(stderr,"Probleme dans l'envoie du ACK");
						}
						
						buffer_len[indexBuf] = pkt_get_length(pkt_recu);
						buffer_payload[indexBuf] = (char *)pkt_get_payload(pkt_recu);
						}
						}
					}
				}
			}
		}
	
	close(sfd);
	close(fd);
	pkt_del(pkt_ack);
	pkt_del(pkt_recu);

}

int send_ack(pkt_t *pkt_ack, int seqnum, int ack, uint32_t timestamp){

	//On va tester pr voir si on sait bien créer la pkt_ack. Si ca fonctionne, en envoie 0 sinon on renvoie -1
	
	pkt_status_code return_status;
	
	return_status = pkt_set_seqnum(pkt_ack, seqnum+1);
	if(return_status != PKT_OK){
		fprintf(stderr,"probleme seqnum");
		return -1;
	}
	
	return_status = pkt_set_timestamp(pkt_ack, timestamp);
	if(return_status != PKT_OK){
		fprintf(stderr,"probleme timestamp");
		return -1;
	}

	
	if(ack == PTYPE_NACK){
		return_status = pkt_set_type(pkt_ack, PTYPE_NACK);
	}
	if(ack == PTYPE_ACK)
		return_status = pkt_set_type(pkt_ack, PTYPE_ACK);
		
	if(return_status != PKT_OK){
		fprintf(stderr,"probleme type");
		return -1;
	}
	return 0;
}
