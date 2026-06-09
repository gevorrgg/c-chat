#ifndef PACKET_H
#define PACKET_H

#define MAX_CIPHERTEXT_SIZE 1024

struct packet_in
{
	int8_t status;
	uint32_t length;
	uint8_t nonce[crypto_aead_chacha20poly1305_ietf_NPUBBYTES];
	uint8_t ciphertext[MAX_CIPHERTEXT_SIZE];
};


#endif