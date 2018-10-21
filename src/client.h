#ifndef __CLIENT_H_
#define __CLIENT_H_


void read_data(int fd, int sfd);

void send_data(const char *hostname, int port, char *file);

#endif