#include <stddef.h>
#include <sodium.h>

#include "../include/crypto.h"
#include "../include/packet.h"
#include "../include/network.h"

/*
 * Handshake (server side), mirror image of dh_key_exchange_client():
 *   1. generate an ephemeral X25519 key pair for this client;
 *   2. send our public key (32 bytes, raw);
 *   3. receive the client's public key (32 bytes, raw);
 *   4. derive rx/tx with crypto_kx_server_session_keys().
 * Both sides send before they receive, so the two public keys cross on the
 * wire and neither side blocks waiting for the other.
 */
int dh_key_exchange_server(size_t client_socket_fd, uint8_t *rx, uint8_t *tx)
{
    uint8_t server_pk[crypto_kx_PUBLICKEYBYTES];
    uint8_t server_sk[crypto_kx_SECRETKEYBYTES];

    crypto_kx_keypair(server_pk, server_sk);

    if (send_all(client_socket_fd, server_pk, crypto_kx_PUBLICKEYBYTES) <= 0) // send public keys
	{
		return DH_EXCHANGE_FAILED;
	}

    uint8_t client_pk[crypto_kx_PUBLICKEYBYTES];

    if (recv_all(client_socket_fd, client_pk, crypto_kx_PUBLICKEYBYTES) <= 0) // receive client's public keys
	{
		return DH_EXCHANGE_FAILED;
	}

    // fails only if client_pk is not a valid curve point
    if (crypto_kx_server_session_keys(rx, tx, server_pk, server_sk, client_pk) != 0)
	{
		return DH_EXCHANGE_FAILED;
	}

	return DH_EXCHANGE_SUCCESS;
}

/*
 * AEAD decryption: libsodium verifies the Poly1305 tag before releasing any
 * plaintext, so a modified or replayed-with-wrong-key packet fails cleanly.
 * No additional data (AD) is used, hence the NULL/0 pair below.
 */
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

	// NOTE: on success the plaintext length is returned, not PACKET_DECRYPTION_SUCCESS
	return (int)(*plaintext_len);
}

int packet_encrypt(const uint8_t *plaintext, size_t plaintext_len, struct packet_in *packet, const uint8_t *tx)
{
	if (!plaintext || !packet || !tx)
	{
		return PACKET_ENCRYPTION_FAILED;
	}

	// generating nonce: must be unique per (key, message); 96 random bits
	// are fine for the message volume of a chat session
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

    // defensive check: clen == plaintext_len + ABYTES, so this only trips
    // if the caller passed a plaintext longer than MAX_PLAINTEXT_LEN
    if (clen > MAX_CIPHERTEXT_SIZE)
    {
        return PACKET_ENCRYPTION_FAILED;
    }

    packet->length = clen;

	return PACKET_ENCRYPTION_SUCCESS;
}
