#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <CUnit/CUnit.h>	
#include <CUnit/Basic.h>
#include <CUnit/TestDB.h>

#include "../src/client.h"
#include "../src/packet_interface.h"
#include "../src/send_receive.h"
#include "../src/server.h"
#include "../src/packet_interface.h"


#define FALSE 0
#define TRUE !FALSE

/* Séries de tests pour le Projet LINGI1341*/


/*Tests sur packet_implem.c*/

void test_pkt_new(void)
{
  pkt_t* pkt = pkt_new();
  CU_ASSERT_PTR_NOT_NULL(pkt);
  pkt_del(pkt);
}

void test_set_type(void)
{
  pkt_t* pkt = pkt_new();
  pkt_status_code status=PKT_OK;

  status=pkt_set_type(pkt,PTYPE_ACK);//on teste le set_type
  CU_ASSERT_EQUAL(status,PKT_OK);//pas d'erreur de retour de la fonction si type correct

  status=pkt_set_type(pkt,10);//on essaye un type qui existe pas
  CU_ASSERT_EQUAL(status,E_TYPE)//on doit avoir une erreur de type

  status=pkt_set_type(pkt,1);//on essaye un type ACK
  CU_ASSERT_EQUAL(pkt_get_type(pkt),PTYPE_DATA);//on a le bon type
  pkt_del(pkt);

}

void test_set_tr(void)
{
  pkt_t* pkt = pkt_new();
  pkt_status_code status=PKT_OK;

  status=pkt_set_tr(pkt,1);//on teste le set_tr en placant tr à 1
  CU_ASSERT_EQUAL(status,PKT_OK);//pas d'erreur de retour de la fonction si tr correct

  CU_ASSERT_EQUAL(pkt_get_type(pkt),PTYPE_DATA)//Comme on a le tr posé à 1, le type doit etre PTYPE_DATA
}

void test_set_window(void)
{
  pkt_t* pkt = pkt_new();
  pkt_status_code status=PKT_OK;

  status=pkt_set_window(pkt,MAX_WINDOW_SIZE-1);//on met une taille de window correcte
  CU_ASSERT_EQUAL(status,PKT_OK);//taille valide
  
  status=pkt_set_window(pkt,MAX_WINDOW_SIZE+1);//on met une taille de window non correcte
  CU_ASSERT_NOT_EQUAL(status,PKT_OK);//taille invalide
}

void test_set_payload(void)
{
	pkt_t* pkt = pkt_new();
	pkt_status_code status=PKT_OK;
	
	char *testpetit = "123456789"; //plus petit que MAX_PAYLOAD_SIZE
	
	char testgrand[2000];
	int i;
	for(i=0;i<2000;i++){testgrand[i]='a';} //plus grand que MAX_PAYLOAD_SIZE
	
	status = pkt_set_payload(pkt,testpetit,10);
	CU_ASSERT_EQUAL(status,PKT_OK);//payload à la bonne taille
	 
	 status = pkt_set_payload(pkt,testgrand,2000);
	 CU_ASSERT_NOT_EQUAL(status,PKT_OK);//payload pas à la bonne taille
	
}

/*
 * Tests sur send_receive
 */

void test_real_address(void) //on teste la creation d'une sockaddr_in6 à partir d'un nom d'hote
{
	struct sockaddr_in6 ipv6;
	const char *testreal1 = real_address("localhost",&ipv6);//localhost est un nom d'adresse connu par l'ordinateur
	CU_ASSERT_EQUAL(NULL,testreal1);
	
	const char *testreal2 = real_address("nomrandom",&ipv6);//nom d'adresse pas reconnu par l'ordinateur
	CU_ASSERT_NOT_EQUAL(NULL,testreal2);
	
	}
	
	void test_create_socket(void) //on teste la creation d'un socket
{
	struct sockaddr_in6 ipv6first;
	const char *test1 = real_address("localhost",&ipv6first);
	CU_ASSERT_EQUAL(NULL,test1);
	
	struct sockaddr_in6 ipv6second;
	const char *test2 = real_address("localhost",&ipv6second);
	CU_ASSERT_EQUAL(NULL,test2);
	
	int fdtest= create_socket(&ipv6first,10000,&ipv6second,12345);//on peut choisir les ports aléatoirement
	CU_ASSERT_NOT_EQUAL(fdtest,-1); //fdtest retournerait -1 s'il y avait une erreur
	
	}


int setup(void)  {return 0;}
int teardown(void) {return 0;}

int main()
{
	if (CUE_SUCCESS != CU_initialize_registry()){return CU_get_error();}
	CU_pSuite pSuite = NULL;
	
	pSuite = CU_add_suite("Tests de packet_implem", setup, teardown);
	
	if(NULL == pSuite) 
	{CU_cleanup_registry();
		return CU_get_error();
		}
	
	if ((NULL == CU_add_test(pSuite, "pkt_new",test_pkt_new)) ||
    (NULL == CU_add_test(pSuite, "set_type", test_set_type)) ||
    (NULL == CU_add_test(pSuite, "set_tr", test_set_tr)) ||
    (NULL == CU_add_test(pSuite, "set_window", test_set_window)) ||
    (NULL == CU_add_test(pSuite, "set_payload", test_set_payload)))
    {
		printf("Tous les tests ne se sont pas effectue avec succes\n");
		CU_cleanup_registry();
		return CU_get_error();
    }
    
    
    pSuite = CU_add_suite("Tests de send_receive", setup, teardown);
	
	if(NULL == pSuite) 
	{CU_cleanup_registry();
		return CU_get_error();
		}
		
		if ((NULL == CU_add_test(pSuite, "real_adress",test_real_address))||
		(NULL == CU_add_test(pSuite, "create socket",test_create_socket))
		)
    {
		printf("Tous les tests ne se sont pas effectue avec succes\n");
		CU_cleanup_registry();
		return CU_get_error();
    }
    
    
	printf("\nVoici les resultat de nos de tests:\n");
	printf("\nChaque suite correspond à un fichier de notre dossier src\n");
	printf("Chaque test correspond a une fonction.\n");
	printf("Il peut y avoir un ou plusieurs asserts par test.\n");
	printf("Seules les suites contenant des erreurs apparaitront dans les détails.\n");
	
	CU_basic_run_tests();
	CU_basic_show_failures(CU_get_failure_list());
	CU_cleanup_registry();
	printf("\n");
		return 0;		
}
