#include "packet_interface.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <errno.h>
#include <zlib.h>

struct __attribute__((__packed__)) pkt {
	uint8_t window : 5; 
	uint8_t tr : 1;
	uint8_t type : 2;
	uint8_t seqnum;
	uint16_t length;
	uint32_t timestamp;
	uint32_t CRC1;
	char *payload;
	uint32_t CRC2;
};

void main(){
	printf("yes \n");
}

pkt_t* pkt_new()
{
	pkt_t *new_pkt = (pkt_t *) malloc(sizeof(pkt_t));
	if(new_pkt == NULL){
		perror("ERREUR lors de l'allocation de mémoire du package \n");
		return NULL; 	
	}

	new_pkt->payload = (char *) malloc(sizeof(char)*MAX_PAYLOAD_SIZE);
	if(new_pkt->payload == NULL) {
		free(new_pkt);
		perror("ERREUR lors l'allocation de mémoire du payload \n");
		return NULL;
	}

	new_pkt->window = 0;
	new_pkt->tr = 0; 
	new_pkt->type = 1;
	new_pkt->seqnum = 0; 
	new_pkt->length = htons(0); 
	new_pkt->timestamp = 0;
	new_pkt->CRC1 = htonl(0);
	new_pkt->CRC2 = htonl(0);
	return new_pkt;
}

void pkt_del(pkt_t *pkt)
{
    free(pkt->payload);
    free(pkt);
}

pkt_status_code pkt_decode(const char *data, const size_t len, pkt_t *pkt)
{
	/* Vérification de la taille du paquet */
	if(len == 0) return E_UNCONSISTENT;

	uint8_t header = data[0];
	pkt_status_code status; 


	/********************
		*	HEADER  *    
	********************/

	/* Vérification de la taille du paquet */
	if(len < 8) return E_NOHEADER;

	// Type  // 2 1er bits du 1er octet 
	status = pkt_set_type(pkt,header >> 6);
	if(status != PKT_OK) return status; 

	/* TR - 3eme bit du 1er octet */
	status = pkt_set_tr(pkt,0); // on le met à zéro avant le recalcul du CRC1 
	if(status != PKT_OK) return status;

	/* Window - 5 derniers bits du 1er octet */ 
	status = pkt_set_window(pkt,(uint8_t) (header<<3) >> 3); //pour garder les 5 derniers bits 
	if(status != PKT_OK) return status; 

	/* Seqnum - 2eme octet */
	status = pkt_set_seqnum(pkt,data[1]);
	if(status != PKT_OK) return status; 

	/* Length - 3eme et 4eme octets */
	uint16_t length = ntohs(*(uint16_t *)(data+2));
	status = pkt_set_length(pkt,length);
	if(status != PKT_OK) return status;

	/* Timestamp - 5eme à 8eme octets */
	status = pkt_set_timestamp(pkt,*(uint32_t *)(data+4)); 





	/********************
	   * CRC/PAYLOAD  *    
	********************/

	/* Vérification de la taille du paquet */
	if(len < 12) return E_UNCONSISTENT;

	/* CRC1 - 9eme à 12eme octets */
	uint32_t CRC1 = ntohl(*((uint32_t *)(data+8)));
	uint32_t newCRC1 = crc32(0L, Z_NULL, 0);
	newCRC1 = crc32(newCRC1 , (const Bytef *) data, 8);
	if(newCRC1 != CRC1) return E_CRC;

	status = pkt_set_crc1(pkt,newCRC1);

	// Maintenant qu'on a vérifié le CRC, on peut mettre tr à jour 


	/* Payload - taille variable : de 0 à 512 octets */
	if(length > 0){
		/* Vérification de la taille du paquet */
		if(len < 12 + (size_t) length) return E_UNCONSISTENT;
		status = pkt_set_payload(pkt,&(data[12]),length);
		if(status != PKT_OK) return status; 
	}

	/* CRC2 - 4 derniers octets */
	if(length > 0 && pkt_get_tr(pkt) == 0){
		/* Vérification de la taille du paquet */
		if(len < 16 + (size_t) length) return E_UNCONSISTENT;

		uint32_t CRC2 = ntohl(*((uint32_t *)(data+12+length)));
		const char *payload = pkt_get_payload(pkt);

		uint32_t newCRC2 = crc32(0L, Z_NULL, 0);
		newCRC2 = crc32(newCRC2, (const Bytef *)payload, length);
		if(CRC2 != newCRC2) return E_CRC;

		status = pkt_set_crc2(pkt,CRC2);

	}

	return PKT_OK;
}

pkt_status_code pkt_encode(const pkt_t* pkt, char *buf, size_t *len)
{
	/* Vérification de la taille du buffer */
	uint16_t length = pkt_get_length(pkt);
	if(pkt_get_tr(pkt) == 0 && length > 0){
		if(*len < 16 + (size_t) length) return E_NOMEM;
		*len = 16 + (size_t) length; 
	}
	else if(*len < 12) return E_NOMEM;
	else *len = 12;


	/* Type | tr | window */
	ptypes_t type = pkt_get_type(pkt);
	type = type << 6;
	uint8_t tr = pkt_get_tr(pkt);
	tr = tr << 5;
	uint8_t window = pkt_get_window(pkt);
	buf[0] = type | tr | window; 

	/* Seqnum - length - timestamp */
	char *p = (char *) pkt;
	size_t i;
	for(i = 1; i < 8; i++){
		buf[i] = p[i];
	}

	/* CRC1 */
	uint32_t CRC1 = crc32(0L, Z_NULL, 0);
	CRC1 = crc32(CRC1, (const Bytef *) buf, 8);
	*(uint32_t *) (buf+8) = htonl(CRC1);

	/* Payload */
	const char *payload = pkt_get_payload(pkt);
	for(i = 0; i < length; i++){
		buf[12+i] = payload[i];
	}

	/* CRC2 */
	if(length > 0 && pkt_get_tr(pkt) == 0){
		uint32_t CRC2 = crc32(0L, Z_NULL, 0);
		CRC2 = crc32(CRC2, (const Bytef *) payload, length);
		*(uint32_t *) (buf+12+length) = htonl(CRC2);
	}


	return PKT_OK;
}

ptypes_t pkt_get_type  (const pkt_t* pkt)
{
	return pkt->type;
}

uint8_t  pkt_get_tr(const pkt_t* pkt)
{
	return pkt->tr;
}

uint8_t  pkt_get_window(const pkt_t* pkt)
{
	return pkt->window;
}

uint8_t  pkt_get_seqnum(const pkt_t* pkt)
{
	return pkt->seqnum;
}

uint16_t pkt_get_length(const pkt_t* pkt)
{
	if(pkt_get_tr(pkt) == 0){
		return ntohs(pkt->length);
	}
	return 0;
}
uint32_t pkt_get_timestamp(const pkt_t* pkt)
{
	return pkt->timestamp;
}

uint32_t pkt_get_crc1(const pkt_t* pkt)
{
	return ntohl(pkt->CRC1);
}

uint32_t pkt_get_crc2(const pkt_t* pkt)
{
	if(pkt_get_tr(pkt) == 0){ // pkt_get_length(pkt) != 0
		return ntohl(pkt->CRC2);
	}
	return 0; 
}

const char* pkt_get_payload(const pkt_t* pkt)
{
	if(pkt_get_length(pkt) == 0){ // pkt_get_tr(pkt) != 0 
		return NULL;
	}
	return pkt->payload; 
}


pkt_status_code pkt_set_type(pkt_t *pkt, const ptypes_t type)
{
	if(type == PTYPE_DATA || type == PTYPE_ACK || type == PTYPE_NACK){
		pkt->type = type;
		return PKT_OK;
	}
	return E_TYPE;
}

pkt_status_code pkt_set_tr(pkt_t *pkt, const uint8_t tr)
{
	if(pkt_get_type(pkt) == PTYPE_DATA){
		pkt->tr = tr; 
		return PKT_OK;
	}
	return E_TR;
}

pkt_status_code pkt_set_window(pkt_t *pkt, const uint8_t window)
{
	if(window > MAX_WINDOW_SIZE)
		return E_WINDOW;
	pkt->window = window; 
	return PKT_OK;
}

pkt_status_code pkt_set_seqnum(pkt_t *pkt, const uint8_t seqnum)
{
	pkt->seqnum = seqnum; 
	return PKT_OK;
}

pkt_status_code pkt_set_length(pkt_t *pkt, const uint16_t length)
{
	if(length > MAX_PAYLOAD_SIZE){
		return E_LENGTH;
	}
	pkt->length = htons(length); 
	return PKT_OK;
}

pkt_status_code pkt_set_timestamp(pkt_t *pkt, const uint32_t timestamp)
{
	pkt->timestamp = timestamp;
	return PKT_OK;
}

pkt_status_code pkt_set_crc1(pkt_t *pkt, const uint32_t crc1)
{
	pkt->CRC1 = htonl(crc1);
	return PKT_OK;
}

pkt_status_code pkt_set_crc2(pkt_t *pkt, const uint32_t crc2)
{
	pkt->CRC2 = htonl(crc2);
	return PKT_OK;
}

pkt_status_code pkt_set_payload(pkt_t *pkt,
							    const char *data,
								const uint16_t length)
{
	pkt_status_code status_length = pkt_set_length(pkt, length);
	if(status_length == PKT_OK){
		pkt->payload = realloc(pkt->payload, length);
		memcpy(pkt->payload, data, length);
	}
	return status_length; 
}