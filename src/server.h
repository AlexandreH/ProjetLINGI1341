#ifndef __SERVER_H_
#define __SERVER_H_

#include "packet_interface.h"

/* Block the caller until a message is received on sfd,
 * and connect the socket to the source addresse of the received message
 * @sfd: a file descriptor to a bound socket but not yet connected
 * @return: 0 in case of success, -1 otherwise
 * @POST: This call is idempotent, it does not 'consume' the data of the message,
 * and could be repeated several times blocking only at the first call.
 */
int wait_for_client(int sfd);


/* Reçois des données venant d'une adresse à l'aide du socket tout en 
 * envoyant des données à cette même adresse (des acquittements)
 * Elle utilise les paquets 
 *
 * @fd: un file descriptor vers un fichier 
 * @sfd: un socket connecté à une adresse de réception et à un port 
 * @return : -1 en cas d'erreur, 0 sinon 
 */
int read_write_loop(int fd, int sfd);

/* Cette fonction : 
 * - converti l'adresse hostname en une adresse IPv6 
 *		utilisable par le système
 * - créé un socket et le connecte à l'adresse hostname et au port 
 * - ouvre le fichier file en écriture  
 * 		(STDOUT si file == NULL)
 * - imprime un message d'erreur sur la sortie d'erreur 
 * 		standard en cas de problème 
 *
 * @hostname : un string d'une adresse réseau 
 * @port : le numéro d'un port 
 * @file : un fichier (NULL si on écrit sur la sortie standard)
 * @fd : où sera stocké le file descriptor du fichier 
 * @sfd : où sera stocké le socket connecté 
 * @return : -1 en cas d'erreur, 0 sinon 
 */ 
int receive_data(const char *hostname, int port, char *file, int *fd, int *sfd);

/* Cette fonction rempli un paquet avec les données d'un acquittement,
 * l'encode dans un buffer, et l'envoie sur le socket 
 *
 * @pkt : le paquet à remplir 
 * @sfd : le socket où envoyer l'acquittement 
 * @type : le type d'acquittement (NACK ou ACK)
 * @seqnum : le numéro de séquence reçu 
 * @timestamp : la donnée à mettre dans le timestamp du paquet
 * @window : la taille de la window du receiver  
 * @return : -1 en cas d'erreur, 0 sinon 
 */ 
int send_ack(pkt_t * pkt, int sfd, int type, int seqnum, uint32_t timestamp, int window);
#endif
