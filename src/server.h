#ifndef __SERVER_H_
#define __SERVER_H_

/* Block the caller until a message is received on sfd,
 * and connect the socket to the source addresse of the received message
 * @sfd: a file descriptor to a bound socket but not yet connected
 * @return: 0 in case of success, -1 otherwise
 * @POST: This call is idempotent, it does not 'consume' the data of the message,
 * and could be repeated several times blocking only at the first call.
 */
 
#include "packet_interface.h"

int wait_for_client(int sfd);

void receive_data(const char *hostname, int port, char *file);

int send_ack(pkt_t *pkt_ack, int seqnum, int ack, uint32_t time_data);
#endif
