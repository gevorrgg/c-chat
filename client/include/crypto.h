#ifndef CRYPTO_H
#define CRYPTO_H

#include <sodium.h>

typedef enum
{
	DH_EXCHANGE_SUCCESS = 0,
	DH_EXCHANGE_FAILED = -1
} dh_exchange_status;

typedef enum
{
	PACKET_DECRYPTION_FAILED = -1,
	PACKET_DECRYPTION_SUCCESS = 0
} packet_decrypt_status;

typedef enum
{
    PACKET_ENCRYPTION_SUCCESS = 0,
    PACKET_ENCRYPTION_FAILED = -1
} packet_encrypt_status;

struct packet_in;

int dh_key_exchange_client(size_t client_socket_fd, uint8_t *rx, uint8_t *tx);
int packet_decrypt(const struct packet_in *packet, uint8_t *plaintext, unsigned long long *plaintext_len, const uint8_t *rx);
int packet_encrypt(const uint8_t *plaintext, size_t plaintext_len, struct packet_in *packet, const uint8_t *tx);

#endif