#ifndef __CLIENT_H_
#define __CLIENT_H_

/* Lis sur le fichier à l'aide du file descriptor et envoie les 
 * données  à une adresse à l'aide du socket tout en 
 * recevant des données de cette même adresse (des acquittements)
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
 * - ouvre le fichier file en lecture 
 * 		(STDIN si file == NULL)
 * - imprime un message d'erreur sur la sortie d'erreur 
 * 		standard en cas de problème 
 *
 * @hostname : un string d'une adresse réseau 
 * @port : le numéro d'un port 
 * @file : un fichier (NULL si on lit sur l'entrée standard)
 * @fd : où sera stocké le file descriptor du fichier 
 * @sfd : où sera stocké le socket connecté 
 * @return : -1 en cas d'erreur, 0 sinon 
 */

int send_data(const char *hostname, int port, char *file, int *fd, int *sfd);

#endif