#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/time.h>

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


/*Premiere serie de tests sur packet_implem.c*/

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

void test_4(void)
{
  CU_ASSERT(FALSE);
}

void test_5(void)
{
  CU_ASSERT_STRING_EQUAL("string aaa", "string aaa");
}
int setup(void)  {return 0;}
int teardown(void) {return 0;}

int main(int argc, char **argv)
{
	if (CUE_SUCCESS != CU_initialize_registry()){return CU_get_error();}
	CU_pSuite pSuite = NULL;
	pSuite = CU_add_suite("Premiere suite de tests", setup, teardown);
	if(NULL == pSuite) 
	{//CU_cleanup_registry();
		return CU_get_error();
		}
	
	if ((NULL == CU_add_test(pSuite, "isTrue",test_pkt_new)) ||
    (NULL == CU_add_test(pSuite, "isNotEqual", test_set_type)) ||
    (NULL == CU_add_test(pSuite, "isEqualString", test_set_tr)) ||
    (NULL == CU_add_test(pSuite, "IsFalse", test_4)) ||
    (NULL == CU_add_test(pSuite, "isEqualString", test_5)))
    {
		printf("Tous les tests ne se sont pas effectue avec succes\n");
		//CU_cleanup_registry();
		return CU_get_error();
    }
	printf("\nVoici les resultat de notre premiere suite de tests:\n");
	CU_basic_run_tests();
	CU_basic_show_failures(CU_get_failure_list());
	//CU_cleanup_registery();
	printf("\n");
		return 0;		
}
