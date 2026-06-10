#ifndef PACKET_H
#define PACKET_H

#include <stdint.h>
#include <sodium.h>

#define MAX_PLAINTEXT_LEN 1024
#define MAX_CIPHERTEXT_SIZE (MAX_PLAINTEXT_LEN + crypto_aead_chacha20poly1305_ietf_ABYTES)

struct packet_in
{
	uint32_t length;
	uint8_t nonce[crypto_aead_chacha20poly1305_ietf_NPUBBYTES];
	uint8_t ciphertext[MAX_CIPHERTEXT_SIZE];
};


#endif