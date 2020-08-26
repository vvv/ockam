#include <string.h>
#include <stdio.h>

#include "ockam/error.h"
#include "ockam/key_agreement.h"
#include "ockam/key_agreement/impl.h"
#include "ockam/log.h"
#include "ockam/vault.h"
#include "ockam/vault/default.h"
#include "xx_local.h"

extern ockam_memory_t* gp_ockam_key_memory;

ockam_key_dispatch_table_t xx_key_dispatch = {
  ockam_key_establish_initiator_xx, ockam_key_establish_responder_xx, xx_encrypt, xx_decrypt, xx_key_deinit
};

ockam_error_t ockam_xx_key_initialize(ockam_key_t*    p_key,
                                      ockam_memory_t* p_memory,
                                      ockam_vault_t*  p_vault,
                                      ockam_reader_t* p_reader,
                                      ockam_writer_t* p_writer)
{
  ockam_error_t   error    = OCKAM_ERROR_NONE;
  ockam_xx_key_t* p_xx_key = NULL;

  if (!p_key || !p_vault || !p_reader || !p_writer || !p_memory) {
    error = KEYAGREEMENT_ERROR_PARAMETER;
    goto exit;
  }

  gp_ockam_key_memory = p_memory;

  p_key->dispatch = &xx_key_dispatch;
  error           = ockam_memory_alloc_zeroed(p_memory, &p_key->context, sizeof(ockam_xx_key_t));
  if (error) goto exit;
  p_xx_key           = (ockam_xx_key_t*) p_key->context;
  p_xx_key->p_vault  = p_vault;
  p_xx_key->p_reader = p_reader;
  p_xx_key->p_writer = p_writer;

exit:
  if (error) {
    ockam_log_error("%x", error);
    if (p_key) {
      if (p_key->context) ockam_memory_free(p_memory, p_key->context, 0);
    }
  }
  return error;
}

ockam_error_t
xx_encrypt(void* p_context, uint8_t* payload, size_t payload_size, uint8_t* msg, size_t msg_length, size_t* msg_size)
{
  ockam_error_t   error = OCKAM_ERROR_NONE;
  uint8_t         cipher_text_and_tag[MAX_XX_TRANSMIT_SIZE];
  size_t          ciphertext_and_tag_length;
  ockam_xx_key_t* p_xx_key = (ockam_xx_key_t*) p_context;

  if (msg_length < (payload_size + TAG_SIZE)) {
    error = TRANSPORT_ERROR_BUFFER_TOO_SMALL;
    goto exit;
  }

  ockam_memory_set(gp_ockam_key_memory, cipher_text_and_tag, 0, sizeof(cipher_text_and_tag));
  error = ockam_vault_aead_aes_gcm_encrypt(p_xx_key->p_vault,
                                           &p_xx_key->encrypt_secret,
                                           p_xx_key->encrypt_nonce,
                                           NULL,
                                           0,
                                           payload,
                                           payload_size,
                                           cipher_text_and_tag,
                                           sizeof(cipher_text_and_tag),
                                           &ciphertext_and_tag_length);
  ;
  ockam_memory_copy(gp_ockam_key_memory, msg, cipher_text_and_tag, ciphertext_and_tag_length);
  p_xx_key->encrypt_nonce += 1;
  *msg_size = ciphertext_and_tag_length;

exit:
  if (error) ockam_log_error("%x", error);
  return error;
}

ockam_error_t xx_decrypt(void*    p_context,
                         uint8_t* payload,
                         size_t   payload_size,
                         uint8_t* cipher_text,
                         size_t   cipher_text_length,
                         size_t*  payload_length)
{
  ockam_error_t   error = OCKAM_ERROR_NONE;
  uint8_t         clear_text[MAX_XX_TRANSMIT_SIZE];
  size_t          clear_text_length = 0;
  ockam_xx_key_t* p_xx_key          = (ockam_xx_key_t*) p_context;

  ockam_memory_set(gp_ockam_key_memory, clear_text, 0, sizeof(clear_text));

  error           = ockam_vault_aead_aes_gcm_decrypt(p_xx_key->p_vault,
                                           &p_xx_key->decrypt_secret,
                                           p_xx_key->decrypt_nonce,
                                           NULL,
                                           0,
                                           cipher_text,
                                           cipher_text_length,
                                           clear_text,
                                           sizeof(clear_text),
                                           &clear_text_length);
  *payload_length = clear_text_length;

  ockam_memory_copy(gp_ockam_key_memory, payload, clear_text, clear_text_length);
  p_xx_key->decrypt_nonce += 1;

exit:
  if (error) ockam_log_error("%x", error);
  return error;
}

ockam_error_t xx_key_deinit(void* p_context)
{
  ockam_error_t   error        = OCKAM_ERROR_NONE;
  ockam_error_t   return_error = OCKAM_ERROR_NONE;
  ockam_xx_key_t* p_xx_key     = (ockam_xx_key_t*) p_context;

  error = ockam_vault_secret_destroy(p_xx_key->p_vault, &p_xx_key->encrypt_secret);
  if (error) return_error = error;
  error = ockam_vault_secret_destroy(p_xx_key->p_vault, &p_xx_key->decrypt_secret);
  if (error) return_error = error;
  ockam_memory_free(gp_ockam_key_memory, p_xx_key, 0);
exit:
  return return_error;
}

ockam_error_t key_agreement_prologue_xx(key_establishment_xx* xx)
{
  ockam_error_t                   error             = OCKAM_ERROR_NONE;
  ockam_vault_secret_attributes_t secret_attributes = { PRIVATE_KEY_SIZE,
                                                        OCKAM_VAULT_SECRET_TYPE_P256_PRIVATEKEY,
                                                        OCKAM_VAULT_SECRET_PURPOSE_KEY_AGREEMENT,
                                                        OCKAM_VAULT_SECRET_EPHEMERAL };
  size_t                          key_size          = 0;
  uint8_t                         ck[32];

  // 1. Generate a static 25519 keypair for this handshake and set it to s
  error = ockam_vault_secret_generate(xx->vault, &xx->s_secret, &secret_attributes);
  if (error) goto exit;

  error = ockam_vault_secret_publickey_get(xx->vault, &xx->s_secret, xx->s, sizeof(xx->s), &key_size);
  if (error) {
    ockam_log_error("key_agreement_prologue_xx: %x", error);
    goto exit;
  }

  // 2. Generate an ephemeral 25519 keypair for this handshake and set it to e
  error = ockam_vault_secret_generate(xx->vault, &xx->e_secret, &secret_attributes);
  if (error) goto exit;

  error = ockam_vault_secret_publickey_get(xx->vault, &xx->e_secret, xx->e, sizeof(xx->e), &key_size);
  if (error) goto exit;

  // 3. Set k to empty, Set n to 0
  xx->nonce = 0;

  // 4. Set h and ck to 'Noise_XX_25519_AESGCM_SHA256'
  ockam_memory_set(gp_ockam_key_memory, xx->h, 0, SHA256_SIZE);
  ockam_memory_copy(gp_ockam_key_memory, xx->h, PROTOCOL_NAME, PROTOCOL_NAME_SIZE);
  ockam_memory_set(gp_ockam_key_memory, ck, 0, 32);
  ockam_memory_copy(gp_ockam_key_memory, ck, PROTOCOL_NAME, PROTOCOL_NAME_SIZE);
  secret_attributes.type = OCKAM_VAULT_SECRET_TYPE_CHAIN_KEY;
  error                  = ockam_vault_secret_import(xx->vault, &xx->ck_secret, &secret_attributes, ck, 32);
  if (error) goto exit;

  // 5. h = SHA256(h || prologue),
  // prologue is empty
  mix_hash(xx, NULL, 0);

exit:
  if (error) ockam_log_error("%x", error);
  return error;
}

/*------------------------------------------------------------------------------------------------------*
 *          UTILITY FUNCTIONS
 *------------------------------------------------------------------------------------------------------*/
void print_uint8_str(const uint8_t* p, uint16_t size, const char* msg)
{
  printf("\n%s %d bytes: \n", msg, size);
  for (int i = 0; i < size; ++i) printf("%0.2x", *p++);
  printf("\n");
}

ockam_error_t hkdf_dh(key_establishment_xx* xx,
                      ockam_vault_secret_t* salt,
                      ockam_vault_secret_t* privatekey,
                      uint8_t*              peer_publickey,
                      size_t                peer_publickey_length,
                      ockam_vault_secret_t* secret1,
                      ockam_vault_secret_t* secret2,
                      bool is_last)
{
  ockam_error_t        error = OCKAM_ERROR_NONE;
  ockam_vault_secret_t shared_secret;
  ockam_vault_secret_t generated_secrets[2];

  ockam_memory_set(gp_ockam_key_memory, &shared_secret, 0, sizeof(ockam_vault_secret_t));
  ockam_memory_set(gp_ockam_key_memory, generated_secrets, 0, 2 * sizeof(ockam_vault_secret_t));

  // Compute shared secret
  error = ockam_vault_ecdh(xx->vault, privatekey, peer_publickey, peer_publickey_length, &shared_secret);
  if (OCKAM_ERROR_NONE != error) {
    ockam_log_error("failed ockam_vault_ecdh in responder_m2_send: %x", error);
    goto exit;
  }

  ockam_vault_secret_attributes_t attributes = {
    .length = SHA256_SIZE,
    .type = OCKAM_VAULT_SECRET_TYPE_CHAIN_KEY,
    .purpose = is_last ? OCKAM_VAULT_SECRET_PURPOSE_EPILOGUE : OCKAM_VAULT_SECRET_PURPOSE_KEY_AGREEMENT,
    .persistence = OCKAM_VAULT_SECRET_EPHEMERAL,
  };

  generated_secrets[0].attributes = attributes;

  attributes.length = SYMMETRIC_KEY_SIZE;
  attributes.type = OCKAM_VAULT_SECRET_TYPE_AES128_KEY;

  generated_secrets[1].attributes = attributes;

  // ck, k = HKDF( ck, shared_secret )
  error = ockam_vault_hkdf_sha256(xx->vault, salt, &shared_secret, 2, generated_secrets);
  if (OCKAM_ERROR_NONE != error) {
    ockam_log_error("failed ockam_vault_hkdf_sha256 in hkdf_dh: %x", error);
    goto exit;
  }

  ockam_memory_copy(gp_ockam_key_memory, secret1, &generated_secrets[0], sizeof(ockam_vault_secret_t));
  ockam_memory_copy(gp_ockam_key_memory, secret2, &generated_secrets[1], sizeof(ockam_vault_secret_t));

exit:
  return error;
}

void string_to_hex(const uint8_t* hexstring, uint8_t* val, size_t* p_bytes)
{
  const char* pos   = (const char*) hexstring;
  uint32_t    bytes = 0;

  for (size_t count = 0; count < (strlen((const  char*) hexstring) / 2); count++) {
    sscanf(pos, "%2hhx", &val[count]);
    pos += 2;
    bytes += 1;
  }
  if (NULL != p_bytes) *p_bytes = bytes;
}

// FIXME
void mix_hash(key_establishment_xx* xx, uint8_t* p_bytes, uint16_t b_length)
{
  ockam_error_t error;
  uint8_t*      p_h = &xx->h[0];
  uint8_t       string[MAX_XX_TRANSMIT_SIZE];
  uint8_t       hash[SHA256_SIZE];
  size_t        hash_length = 0;

  ockam_memory_set(gp_ockam_key_memory, &hash[0], 0, sizeof(hash));
  ockam_memory_set(gp_ockam_key_memory, &string[0], 0, sizeof(string));
  ockam_memory_copy(gp_ockam_key_memory, &string[0], &p_h[0], SHA256_SIZE);
  ockam_memory_copy(gp_ockam_key_memory, &string[SHA256_SIZE], p_bytes, b_length);
  error = ockam_vault_sha256(xx->vault, string, SHA256_SIZE + b_length, hash, SHA256_SIZE, &hash_length);
  if (error) goto exit;
  ockam_memory_copy(gp_ockam_key_memory, p_h, hash, hash_length);

exit:
  if (error) ockam_log_error("%x", error);
  return;
}

ockam_error_t make_vector(uint64_t nonce, uint8_t* vector)
{
  uint8_t* pv;
  uint8_t* pn = (uint8_t*) &nonce;

  ockam_memory_set(gp_ockam_key_memory, vector, 0, VECTOR_SIZE);
  pv = vector + 4;
  pn += 7;
  for (int i = 7; i >= 0; --i) { *pv++ = *pn--; }
  return OCKAM_ERROR_NONE;
}
