#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "ockam/error.h"
#include "ockam/key_agreement/xx.h"
#include "ockam/key_agreement/xx_local.h"
#include "ockam/key_agreement.h"
#include "ockam/memory.h"
#include "ockam/log.h"
#include "ockam/transport.h"
#include "ockam/transport/socket_tcp.h"
#include "ockam/vault.h"
#include "xx_test.h"

extern bool scripted_xx;

ockam_error_t xx_test_responder_prologue(key_establishment_xx* xx)
{
  ockam_error_t                   error             = OCKAM_ERROR_NONE;
  ockam_vault_secret_attributes_t secret_attributes = { PRIVATE_KEY_SIZE,
                                                        OCKAM_VAULT_SECRET_TYPE_CURVE25519_PRIVATEKEY,
                                                        OCKAM_VAULT_SECRET_PURPOSE_KEY_AGREEMENT,
                                                        OCKAM_VAULT_SECRET_EPHEMERAL };
  uint8_t                         key[PRIVATE_KEY_SIZE];
  size_t                          key_bytes;
  uint8_t                         ck[SYMMETRIC_KEY_SIZE];

  // 1. Pick a static 25519 keypair for this xx and set it to s
  string_to_hex((uint8_t*) RESPONDER_STATIC, key, &key_bytes);
  error = ockam_vault_secret_import(xx->vault, &xx->s_secret, &secret_attributes, key, key_bytes);
  if (error) {
    ockam_log_error("xx_test_responder_prologue: %x", error);
    goto exit;
  }

  error = ockam_vault_secret_publickey_get(xx->vault, &xx->s_secret, xx->s, P256_PUBLIC_KEY_SIZE, &key_bytes);
  if (error) {
    ockam_log_error("xx_test_responder_prologue: %x", error);
    goto exit;
  }

  // 2. Generate an ephemeral 25519 keypair for this xx and set it to e
  string_to_hex((uint8_t*) RESPONDER_EPH, key, &key_bytes);
  secret_attributes.persistence = OCKAM_VAULT_SECRET_EPHEMERAL;
  error = ockam_vault_secret_import(xx->vault, &xx->e_secret, &secret_attributes, key, key_bytes);
  if (error) {
    ockam_log_error("xx_test_responder_prologue: %x", error);
    goto exit;
  }

  error = ockam_vault_secret_publickey_get(xx->vault, &xx->e_secret, xx->e, P256_PUBLIC_KEY_SIZE, &key_bytes);
  if (error) {
    ockam_log_error("xx_test_responder_prologue: %x", error);
    goto exit;
  }

  // Nonce to 0, k to empty
  xx->nonce = 0;
  memset(xx->k, 0, sizeof(xx->k));

  // 4. Set h and ck to 'Noise_XX_25519_AESGCM_SHA256'
  memset(xx->h, 0, SHA256_SIZE);
  memcpy(xx->h, PROTOCOL_NAME, PROTOCOL_NAME_SIZE);
  memset(ck, 0, SYMMETRIC_KEY_SIZE);
  memcpy(ck, PROTOCOL_NAME, PROTOCOL_NAME_SIZE);
  secret_attributes.type = OCKAM_VAULT_SECRET_TYPE_BUFFER;
  error                  = ockam_vault_secret_import(xx->vault, &xx->ck_secret, &secret_attributes, ck, SYMMETRIC_KEY_SIZE);
  if (error) {
    ockam_log_error("xx_test_responder_prologue: %x", error);
    goto exit;
  }

  // 5. h = SHA256(h || prologue),
  // prologue is empty
  mix_hash(xx, NULL, 0);

exit:
  return error;
}

ockam_error_t test_responder_handshake(ockam_key_t* p_key)
{
  ockam_error_t        error = OCKAM_ERROR_INTERFACE_KEYAGREEMENT;
  uint8_t              write_buffer[MAX_XX_TRANSMIT_SIZE];
  uint8_t              read_buffer[MAX_XX_TRANSMIT_SIZE];
  size_t               transmit_size  = 0;
  size_t               bytes_received = 0;
  uint8_t              compare[1024];
  size_t               compare_bytes;
  ockam_xx_key_t*      p_xx_key = (ockam_xx_key_t*) p_key->context;
  key_establishment_xx xx;

  memset(&xx, 0, sizeof(xx));
  xx.vault = p_xx_key->p_vault;

  /* Prologue initializes keys and xx parameters */
  error = xx_test_responder_prologue(&xx);
  if (error) goto exit;

  /* Msg 1 receive */
  error = ockam_read(p_xx_key->p_reader, &read_buffer[0], MAX_XX_TRANSMIT_SIZE, &bytes_received);
  if (error) goto exit;

  /* Msg 1 process */
  error = xx_responder_m1_process(&xx, read_buffer, bytes_received);
  if (error) goto exit;

  /* Msg 2 make */
  error = xx_responder_m2_make(&xx, write_buffer, sizeof(write_buffer), &transmit_size);
  if (error) goto exit;

  /* Msg 2 verify */
  string_to_hex((uint8_t*) MSG_2_CIPHERTEXT, compare, &compare_bytes);
  if (0 != memcmp(write_buffer, compare, compare_bytes)) {
    error = KEYAGREEMENT_ERROR_TEST;
    goto exit;
  }

  /* Msg 2 send */
  error = ockam_write(p_xx_key->p_writer, write_buffer, transmit_size);
  if (error) goto exit;

  /* Msg 3 receive */
  error = ockam_read(p_xx_key->p_reader, read_buffer, MAX_XX_TRANSMIT_SIZE, &bytes_received);
  if (error) goto exit;

  /* Msg 3 process */
  error = xx_responder_m3_process(&xx, read_buffer, bytes_received);
  if (error) goto exit;

  /* Epilogue */
  error = xx_responder_epilogue(&xx, p_xx_key);
  if (TRANSPORT_ERROR_NONE != error) {
    ockam_log_error("Failed responder_epilogue: %x", error);
    goto exit;
  }

exit:
  return error;
}

ockam_error_t establish_responder_connection(ockam_transport_t*  p_transport,
                                             ockam_memory_t*     p_memory,
                                             ockam_ip_address_t* p_address,
                                             ockam_reader_t**    pp_reader,
                                             ockam_writer_t**    pp_writer)
{
  ockam_error_t                           error = OCKAM_ERROR_INTERFACE_KEYAGREEMENT;
  ockam_transport_socket_attributes_t tcp_attributes;

  memcpy(&tcp_attributes.listen_address, p_address, sizeof(ockam_ip_address_t));
  tcp_attributes.p_memory = p_memory;
  error                   = ockam_transport_socket_tcp_init(p_transport, &tcp_attributes);
  if (error) goto exit;

  // Wait for a connection
  error = ockam_transport_accept(p_transport, pp_reader, pp_writer, NULL);
  if (error) {
    ockam_log_error("establish_responder_connection: %x", error);
    goto exit;
  }

  error = OCKAM_ERROR_NONE;

exit:
  return error;
}

ockam_error_t xx_test_responder(ockam_vault_t* p_vault, ockam_memory_t* p_memory, ockam_ip_address_t* ip_address)
{
  ockam_transport_t transport = { 0 };
  ockam_error_t     error     = OCKAM_ERROR_INTERFACE_KEYAGREEMENT;

  uint8_t         write_buffer[MAX_XX_TRANSMIT_SIZE];
  uint8_t         read_buffer[MAX_XX_TRANSMIT_SIZE];
  size_t          transmit_size = 0;
  uint8_t         test[16];
  size_t          test_size;
  uint8_t         test_initiator[TEST_MSG_BYTE_SIZE];
  uint8_t         comp[2048];
  size_t          comp_size;
  ockam_key_t     key;
  ockam_reader_t* p_reader;
  ockam_writer_t* p_writer;

  /*-------------------------------------------------------------------------
   * Establish transport connection with responder
   *-----------------------------------------------------------------------*/
  error = establish_responder_connection(&transport, p_memory, ip_address, &p_reader, &p_writer);
  if (error) goto exit;

  ockam_log_info("Responder connected");

  error = ockam_xx_key_initialize(&key, p_memory, p_vault, p_reader, p_writer);
  if (error) goto exit;

  ockam_log_info("Responder key agreement initialized");

  /*-------------------------------------------------------------------------
   * Perform the secret xx
   * If successful, encrypt/decrypt keys will be established
   *-----------------------------------------------------------------------*/

  if (scripted_xx) {
    error = test_responder_handshake(&key);
  } else {
    error = ockam_key_respond(&key);
  }
  if (error) {
    ockam_log_error("ockam_responder_handshake: %x", error);
    goto exit;
  }

  ockam_log_info("Responder key agreement responded");

  /*-------------------------------------------------------------------------
   * Verify secure channel by sending and receiving a known message
   *-----------------------------------------------------------------------*/

  if (scripted_xx) {
    /* Convert string to hex bytes and encrypt */
    string_to_hex((uint8_t*) TEST_MSG_RESPONDER, test, &test_size);
    error = ockam_key_encrypt(&key, test, test_size, write_buffer, sizeof(write_buffer), &transmit_size);
    if (error) goto exit;
    /* Verify test message ciphertext */
    string_to_hex((uint8_t*) MSG_4_CIPHERTEXT, comp, &comp_size);
    if (0 != memcmp(comp, write_buffer, transmit_size)) {
      error = KEYAGREEMENT_ERROR_FAIL;
      goto exit;
    }
  } else {
    error = ockam_key_encrypt(&key, (uint8_t*) ACK, ACK_SIZE, write_buffer, sizeof(write_buffer), &transmit_size);
    if (error != TRANSPORT_ERROR_NONE) {
      ockam_log_error("responder_epilogue_make failed: %x", error);
      goto exit;
    }
  }

  ockam_log_info("Responder test message encrypted");

  /* Send test message */
  error = ockam_write(p_writer, write_buffer, transmit_size);
  if (error) goto exit;

  ockam_log_info("Responder test message sent");

  /* Receive test message  */
  memset(read_buffer, 0, sizeof(read_buffer));
  error = ockam_read(p_reader, read_buffer, MAX_XX_TRANSMIT_SIZE, &transmit_size);
  if (error) goto exit;

  ockam_log_info("Responder test message receiver");

  /* Decrypt test message */

  error = ockam_key_decrypt(&key, test, TEST_MSG_BYTE_SIZE, read_buffer, transmit_size, &test_size);
  if (error) goto exit;

  ockam_log_info("Responder test message decrypted");

  /* Verify test message */
  if (scripted_xx) {
    string_to_hex((uint8_t*) TEST_MSG_INITIATOR, test_initiator, NULL);
    if (0 != memcmp((void*) test, test_initiator, TEST_MSG_BYTE_SIZE)) {
      error = KEYAGREEMENT_ERROR_FAIL;
      goto exit;
    }
  } else {
    if (0 != memcmp(OK, test, OK_SIZE)) {
      error = KEYAGREEMENT_ERROR_FAIL;
      goto exit;
    }
  }
  error = OCKAM_ERROR_NONE;
exit:
  if (error) ockam_log_error("%x", error);
  ockam_transport_deinit(&transport);
  return error;
}
