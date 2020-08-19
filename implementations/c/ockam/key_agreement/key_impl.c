#include "ockam/error.h"
#include "ockam/log.h"
#include "ockam/key_agreement.h"
#include "ockam/key_agreement/impl.h"
#include "ockam/memory.h"

const char* const OCKAM_KEY_AGREEMENT_INTERFACE_ERROR_DOMAIN = "OCKAM_KEY_AGREEMENT_INTERFACE_ERROR_DOMAIN";

static const ockam_error_t ockam_key_agreement_interface_error_none = {
  OCKAM_ERROR_NONE,
  OCKAM_KEY_AGREEMENT_INTERFACE_ERROR_DOMAIN
};

ockam_memory_t* gp_ockam_key_memory = NULL;

ockam_error_t ockam_key_initiate(ockam_key_t* p_key)
{
  ockam_error_t error = ockam_key_agreement_interface_error_none;

  if (!p_key) {
    error.code = OCKAM_KEY_AGREEMENT_INTERFACE_ERROR_INVALID_PARAM;
    goto exit;
  }

  error = p_key->dispatch->initiate(p_key->context);

exit:
  if (ockam_error_has_error(&error)) ockam_log_error("%s: %d", error.domain, error.code);
  return error;
}

ockam_error_t ockam_key_respond(ockam_key_t* p_key)
{
  ockam_error_t error = ockam_key_agreement_interface_error_none;

  if (!p_key) {
    error.code = OCKAM_KEY_AGREEMENT_INTERFACE_ERROR_INVALID_PARAM;
    goto exit;
  }

  error = p_key->dispatch->respond(p_key->context);

exit:
  if (ockam_error_has_error(&error)) ockam_log_error("%s: %d", error.domain, error.code);
  return error;
}

ockam_error_t ockam_key_encrypt(
  ockam_key_t* p_key, uint8_t* payload, size_t payload_size, uint8_t* msg, size_t msg_size, size_t* msg_length)
{
  ockam_error_t error = ockam_key_agreement_interface_error_none;

  if (!p_key || !payload || !msg || !msg_length) {
    error.code = OCKAM_KEY_AGREEMENT_INTERFACE_ERROR_INVALID_PARAM;
    goto exit;
  }
  if (!payload_size || !msg_size) {
    error.code = OCKAM_KEY_AGREEMENT_INTERFACE_ERROR_INVALID_PARAM;
    goto exit;
  }

  error = p_key->dispatch->encrypt(p_key->context, payload, payload_size, msg, msg_size, msg_length);

exit:
  if (ockam_error_has_error(&error)) ockam_log_error("%s: %d", error.domain, error.code);
  return error;
}

ockam_error_t ockam_key_decrypt(
  ockam_key_t* p_key, uint8_t* payload, size_t payload_size, uint8_t* msg, size_t msg_length, size_t* payload_length)
{
  ockam_error_t error = ockam_key_agreement_interface_error_none;

  if (!p_key) {
    error.code = OCKAM_KEY_AGREEMENT_INTERFACE_ERROR_INVALID_PARAM;
    goto exit;
  }
  if (!payload || !msg || !payload_size || !msg_length || !payload_length) {
    error.code = OCKAM_KEY_AGREEMENT_INTERFACE_ERROR_INVALID_PARAM;
    goto exit;
  }

  error = p_key->dispatch->decrypt(p_key->context, payload, payload_size, msg, msg_length, payload_length);

exit:
  if (ockam_error_has_error(&error)) ockam_log_error("%s: %d", error.domain, error.code);
  return error;
}

ockam_error_t ockam_key_deinit(ockam_key_t* p_key)
{
  ockam_error_t error = ockam_key_agreement_interface_error_none;

  if (!p_key) {
    error.code = OCKAM_KEY_AGREEMENT_INTERFACE_ERROR_INVALID_PARAM;
    goto exit;
  }

  error = p_key->dispatch->deinit(p_key->context);

exit:
  if (ockam_error_has_error(&error)) ockam_log_error("%s: %d", error.domain, error.code);
  return error;
}
