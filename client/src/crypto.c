#include <stddef.h>
#include <sodium.h>

#include "../include/crypto.h"
#include "../include/packet.h"
#include "../include/network.h"

int dh_key_exchange_client(size_t client_socket_fd, uint8_t *rx, uint8_t *tx)
{
    uint8_t client_pk[crypto_kx_PUBLICKEYBYTES];
    uint8_t client_sk[crypto_kx_SECRETKEYBYTES];

    crypto_kx_keypair(client_pk, client_sk);

    if (send_all(client_socket_fd, client_pk, crypto_kx_PUBLICKEYBYTES) <= 0) // send public keys
	{
		return DH_EXCHANGE_FAILED;
	}

    uint8_t server_pk[crypto_kx_PUBLICKEYBYTES];

    if (recv_all(client_socket_fd, server_pk, crypto_kx_PUBLICKEYBYTES) <= 0) // receive server's public keys
	{
		return DH_EXCHANGE_FAILED;
	}

    if (crypto_kx_client_session_keys(rx, tx, client_pk, client_sk, server_pk) != 0)
	{
		return DH_EXCHANGE_FAILED;
	}

	return DH_EXCHANGE_SUCCESS;
}

int packet_decrypt(const struct packet_in *packet, uint8_t *plaintext, unsigned long long *plaintext_len, const uint8_t *rx)
{
	if (!plaintext_len || !packet || !plaintext || !rx)
	{
		return PACKET_DECRYPTION_FAILED;
	}

	if (crypto_aead_chacha20poly1305_ietf_decrypt(
            plaintext, plaintext_len,
            NULL,
            packet->ciphertext, packet->length,
            NULL, 0,
            packet->nonce,
            rx) != 0)
    {
		return PACKET_DECRYPTION_FAILED;
    }

	return (int)(*plaintext_len);
}

int packet_encrypt(const uint8_t *plaintext, size_t plaintext_len, struct packet_in *packet, const uint8_t *tx)
{
	if (!plaintext || !packet || !tx)
	{
		return PACKET_ENCRYPTION_FAILED;
	}

	// generating nonce
	randombytes_buf(packet->nonce, crypto_aead_chacha20poly1305_ietf_NPUBBYTES);

	unsigned long long clen;

	// encrypting
	if (crypto_aead_chacha20poly1305_ietf_encrypt(
            packet->ciphertext, &clen,
            plaintext, plaintext_len,
            NULL, 0,
            NULL,
            packet->nonce,
            tx) != 0)
    {
        return PACKET_ENCRYPTION_FAILED;
    }

    if (clen > MAX_CIPHERTEXT_SIZE)
    {
        return PACKET_ENCRYPTION_FAILED;
    }

    packet->length = clen;

	return PACKET_ENCRYPTION_SUCCESS;
}

