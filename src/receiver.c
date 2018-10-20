#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/time.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h> /* * sockaddr_in6 */
#include <netdb.h>
#include "packet_interface.h"
#include "server.h"


int main (int argc, char **argv){

	int argu; //valeur de retour de getopt
	
	char *fichierbin = NULL; //fichier potentiellement binaire
	int index;
    char *hostname = NULL;
    int port;
    opterr = 0; //empeche le message d'erreur
    
    while ((argu = getopt(argc, argv, "f:")) != -1)// getopt retourne -1 quand plus de caractere d'option
    {switch (argu)
    {
        case 'f':
            fichierbin = optarg; //optarg: pointeur vers l'argument qui suit "f"
            break;
        case '?':
            if (optopt == 'c')
                fprintf (stderr, "Option -%c requires an argument.\n", optopt);
            else if (isprint (optopt))
                fprintf (stderr, "Unknown option `-%c'.\n", optopt);
            else
                fprintf (stderr,"Unknown option character `\\x%x'.\n",optopt);
            return 1;
        default:
            abort ();
    }
}

	if (optind >= argc) {
        fprintf(stderr, "Expected argument after options\n");
        exit(EXIT_FAILURE);
    }
    
     index = optind;//optind vaut l'index du premier elem argv qui n'est pas une option
    index = optind;//optind vaut l'index du premier elem argv qui n'est pas une option
    
    hostname = argv[index];//Hote fourni par l'utilisateur (chaine de caract)
    port = atoi(argv[index+1]);//Port fourni par l'utilisateur (int)
    
	receive_data(hostname, port, fichier);//reception du packet
	return EXIT_SUCCESS;
}
