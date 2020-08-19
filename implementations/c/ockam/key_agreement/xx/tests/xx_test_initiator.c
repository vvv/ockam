
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "ockam/error.h"
#include "ockam/key_agreement.h"
#include "ockam/key_agreement/xx.h"
#include "ockam/key_agreement/xx_local.h"
#include "ockam/memory.h"
#include "ockam/log.h"
#include "ockam/transport.h"
#include "ockam/transport/socket_tcp.h"
#include "ockam/vault.h"
#include "xx_test.h"

extern bool scripted_xx;

ockam_error_t xx_test_initiator_prologue(key_establishment_xx* xx)
{
  ockam_error_t                   error             = ockam_key_agreement_xx_error_none;
  ockam_vault_secret_attributes_t secret_attributes = { KEY_SIZE,
                                                        OCKAM_VAULT_SECRET_TYPE_CURVE25519_PRIVATEKEY,
                                                        OCKAM_VAULT_SECRET_PURPOSE_KEY_AGREEMENT,
                                                        OCKAM_VAULT_SECRET_EPHEMERAL };
  uint8_t                         key[KEY_SIZE];
  size_t                          key_bytes;
  uint8_t                         ck[KEY_SIZE];

  // 1. Pick a static 25519 keypair for this handshake and set it to s

  string_to_hex((uint8_t*) INITIATOR_STATIC, key, &key_bytes);
  error = ockam_vault_secret_import(xx->vault, &xx->s_secret, &secret_attributes, key, key_bytes);
  if (ockam_error_has_error(&error)) {
    ockam_log_error("xx_test_initiator_prologue: %s: %d", error.domain, error.code);
    goto exit;
  }

  error = ockam_vault_secret_publickey_get(xx->vault, &xx->s_secret, xx->s, KEY_SIZE, &key_bytes);
  if (ockam_error_has_error(&error)) {
    ockam_log_error("xx_test_initiator_prologue: %s: %d", error.domain, error.code);
    goto exit;
  }

  // 2. Generate an ephemeral 25519 keypair for this handshake and set it to e

  string_to_hex((uint8_t*) INITIATOR_EPH, key, &key_bytes);
  secret_attributes.persistence = OCKAM_VAULT_SECRET_EPHEMERAL;
  error = ockam_vault_secret_import(xx->vault, &xx->e_secret, &secret_attributes, key, key_bytes);
  if (ockam_error_has_error(&error)) {
    ockam_log_error("xx_test_initiator_prologue: %s: %d", error.domain, error.code);
    goto exit;
  }

  error = ockam_vault_secret_publickey_get(xx->vault, &xx->e_secret, xx->e, KEY_SIZE, &key_bytes);
  if (ockam_error_has_error(&error)) {
    ockam_log_error("xx_test_initiator_prologue: %s: %d", error.domain, error.code);
    goto exit;
  }

  // Nonce to 0, k to empty
  xx->nonce = 0;
  memset(xx->k, 0, sizeof(xx->k));

  // 4. Set h and ck to 'Noise_XX_25519_AESGCM_SHA256'
  memset(xx->h, 0, SHA256_SIZE);
  memcpy(xx->h, PROTOCOL_NAME, PROTOCOL_NAME_SIZE);
  memset(ck, 0, KEY_SIZE);
  memcpy(ck, PROTOCOL_NAME, PROTOCOL_NAME_SIZE);
  secret_attributes.type = OCKAM_VAULT_SECRET_TYPE_BUFFER;
  error                  = ockam_vault_secret_import(xx->vault, &xx->ck_secret, &secret_attributes, ck, KEY_SIZE);
  if (ockam_error_has_error(&error)) goto exit;

  // 5. h = SHA256(h || prologue),
  // prologue is empty
  mix_hash(xx, NULL, 0);

exit:
  if (ockam_error_has_error(&error)) ockam_log_error("%s: %d", error.domain, error.code);
  return error;
}

ockam_error_t test_initiator_handshake(ockam_key_t* p_key)
{
  ockam_error_t        error = ockam_key_agreement_xx_error_none;
  uint8_t              write_buffer[MAX_XX_TRANSMIT_SIZE];
  uint8_t              read_buffer[MAX_XX_TRANSMIT_SIZE];
  size_t               bytes_received = 0;
  size_t               transmit_size  = 0;
  uint8_t              compare[1024];
  size_t               compare_bytes;
  ockam_xx_key_t*      p_xx_key = (ockam_xx_key_t*) p_key->context;
  key_establishment_xx xx;

  memset(&xx, 0, sizeof(xx));
  xx.vault = p_xx_key->p_vault;

  /* Prologue initializes keys and handshake parameters */
  error = xx_test_initiator_prologue(&xx);
  if (ockam_error_has_error(&error)) goto exit;

  // Step 1 generate message
  error = xx_initiator_m1_make(&xx, write_buffer, MAX_XX_TRANSMIT_SIZE, &transmit_size);
  if (ockam_error_has_error(&error)) goto exit;

  // Verify
  string_to_hex((uint8_t*) MSG_1_CIPHERTEXT, compare, &compare_bytes);
  if (0 != memcmp(write_buffer, compare, compare_bytes)) {
    error.code = -1;
    goto exit;
  }

  // Step 1 send message
  error = ockam_write(p_xx_key->p_writer, write_buffer, transmit_size);
  if (ockam_error_has_error(&error)) goto exit;

  // Msg 2 receive
  error = ockam_read(p_xx_key->p_reader, read_buffer, sizeof(read_buffer), &bytes_received);
  if (ockam_error_has_error(&error)) goto exit;

  // Msg 2 process
  error = xx_initiator_m2_process(&xx, read_buffer, bytes_received);
  if (ockam_error_has_error(&error)) goto exit;

  // Msg 3 make
  error = xx_initiator_m3_make(&xx, write_buffer, &transmit_size);
  if (ockam_error_has_error(&error)) goto exit;

  /* Msg 3 verify */
  string_to_hex((uint8_t*) MSG_3_CIPHERTEXT, compare, &compare_bytes);
  if (0 != memcmp(compare, write_buffer, transmit_size)) {
    error.code = -1;
    goto exit;
  }

  // Msg 3 send
  error = ockam_write(p_xx_key->p_writer, write_buffer, transmit_size);
  if (ockam_error_has_error(&error)) goto exit;

  error = xx_initiator_epilogue(&xx, p_xx_key);
  if (ockam_error_has_error(&error)) goto exit;

exit:
  if (ockam_error_has_error(&error)) ockam_log_error("%s: %d", error.domain, error.code);
  return error;
}

ockam_error_t establish_initiator_transport(ockam_transport_t*  p_transport,
                                            ockam_memory_t*     p_memory,
                                            ockam_ip_address_t* ip_address,
                                            ockam_reader_t**    pp_reader,
                                            ockam_writer_t**    pp_writer)
{
  ockam_error_t                           error = ockam_key_agreement_xx_error_none;
  ockam_transport_socket_attributes_t tcp_attrs;
  memset(&tcp_attrs, 0, sizeof(tcp_attrs));
  tcp_attrs.p_memory = p_memory;

  error = ockam_transport_socket_tcp_init(p_transport, &tcp_attrs);
  if (ockam_error_has_error(&error)) {
    ockam_log_error("establish_initiator_transport: %s: %d", error.domain, error.code);
    goto exit;
  }

  error = ockam_transport_connect(p_transport, pp_reader, pp_writer, ip_address, 10, 1);
  if (ockam_error_has_error(&error)) goto exit;

exit:
  return error;
}

ockam_error_t xx_test_initiator(ockam_vault_t* p_vault, ockam_memory_t* p_memory, ockam_ip_address_t* ip_address)
{
  ockam_transport_t transport = { 0 };

  ockam_error_t   error = ockam_key_agreement_xx_error_none;
  uint8_t         write_buffer[MAX_XX_TRANSMIT_SIZE];
  uint8_t         read_buffer[MAX_XX_TRANSMIT_SIZE];
  size_t          bytes_received = 0;
  size_t          transmit_size  = 0;
  uint8_t         test[TEST_MSG_CIPHER_SIZE];
  size_t          test_bytes;
  uint8_t         test_responder[TEST_MSG_CIPHER_SIZE];
  ockam_key_t     key;
  ockam_reader_t* p_reader;
  ockam_writer_t* p_writer;

  error = establish_initiator_transport(&transport, p_memory, ip_address, &p_reader, &p_writer);
  if (ockam_error_has_error(&error)) goto exit;

  printf("Initiator connected\n");

  error = ockam_xx_key_initialize(&key, p_memory, p_vault, p_reader, p_writer);
  if (ockam_error_has_error(&error)) goto exit;

  if (scripted_xx) {
    error = test_initiator_handshake(&key);
  } else {
    error = ockam_key_initiate(&key);
  }
  if (ockam_error_has_error(&error)) goto exit;

  /*-------------------------------------------------------------------------
   * Receive the test message
   *-----------------------------------------------------------------------*/
  error = ockam_read(p_reader, read_buffer, sizeof(read_buffer), &bytes_received);
  if (ockam_error_has_error(&error)) goto exit;

  /*-------------------------------------------------------------------------
   * Confirm the test message
   *-----------------------------------------------------------------------*/
  error = ockam_key_decrypt(&key, test, TEST_MSG_CIPHER_SIZE, read_buffer, bytes_received, &test_bytes);
  if (ockam_error_has_error(&error)) goto exit;

  if (scripted_xx) {
    string_to_hex((uint8_t*) TEST_MSG_RESPONDER, test_responder, NULL);
    if (0 != memcmp((void*) test, test_responder, TEST_MSG_BYTE_SIZE)) { error.code = -1; }
  } else {
    if (0 != memcmp(ACK, test, ACK_SIZE)) { error.code = -1; }
  }
  if (ockam_error_has_error(&error)) goto exit;

  /*-------------------------------------------------------------------------
   * Make the test message
   *-----------------------------------------------------------------------*/
  if (scripted_xx) {
    string_to_hex((uint8_t*) TEST_MSG_INITIATOR, test, &test_bytes);
    error = ockam_key_encrypt(&key, test, test_bytes, write_buffer, sizeof(write_buffer), &transmit_size);
  } else {
    error = ockam_key_encrypt(&key, (uint8_t*) OK, OK_SIZE, write_buffer, sizeof(write_buffer), &transmit_size);
  }
  if (ockam_error_has_error(&error)) goto exit;

  /*-------------------------------------------------------------------------
   * Confirm the test message
   *-----------------------------------------------------------------------*/
  if (scripted_xx) {
    string_to_hex((uint8_t*) MSG_5_CIPHERTEXT, test, &test_bytes);
    if (0 != memcmp(test, write_buffer, transmit_size)) {
      error.code = -1;
      ockam_log_error("Msg 5 failed: %s: %d", error.domain, error.code);
      goto exit;
    }
  }

  /*-------------------------------------------------------------------------
   * Send the test message
   *-----------------------------------------------------------------------*/
  error = ockam_write(p_writer, write_buffer, transmit_size);
  if (ockam_error_has_error(&error)) {
    ockam_log_error("ockam_SendBlocking failed on test message: %s: %d", error.domain, error.code);
    goto exit;
  }

exit:
  if (ockam_error_has_error(&error)) ockam_log_error("%s: %d", error.domain, error.code);
  ockam_transport_deinit(&transport);
  return error;
}
