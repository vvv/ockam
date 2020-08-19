#ifndef OCKAM_CHANNEL_H
#define OCKAM_CHANNEL_H

#include "ockam/error.h"
#include "ockam/io.h"
#include "ockam/memory.h"
#include "ockam/vault.h"

extern const char* const OCKAM_CHANNEL_INTERFACE_ERROR_DOMAIN;

typedef enum {
  OCKAM_CHANNEL_INTERFACE_ERROR_INVALID_PARAM   = 1,
  OCKAM_CHANNEL_INTERFACE_ERROR_NOT_IMPLEMENTED = 2,
  OCKAM_CHANNEL_INTERFACE_ERROR_KEY_AGREEMENT   = 3,
  OCKAM_CHANNEL_INTERFACE_ERROR_STATE           = 4,
} ockam_error_code_channel_interface_t;

typedef struct ockam_channel_t ockam_channel_t;

typedef struct ockam_channel_attributes_t {
  ockam_reader_t* reader;
  ockam_writer_t* writer;
  ockam_memory_t* memory;
  ockam_vault_t*  vault;
} ockam_channel_attributes_t;

ockam_error_t ockam_channel_init(ockam_channel_t* channel, ockam_channel_attributes_t* p_attrs);
ockam_error_t ockam_channel_connect(ockam_channel_t* p_channel, ockam_reader_t** p_reader, ockam_writer_t** p_writer);
ockam_error_t ockam_channel_accept(ockam_channel_t* p_channel, ockam_reader_t** p_reader, ockam_writer_t** p_writer);
ockam_error_t ockam_channel_deinit(ockam_channel_t* channel);

#endif
