#ifndef OCKAM_TRANSPORT_H
#define OCKAM_TRANSPORT_H

#include <stdint.h>
#include "ockam/error.h"
#include "ockam/io.h"
#include "ockam/transport/impl.h"
#include "ockam/memory.h"

extern const char* const OCKAM_TRANSPORT_INTERFACE_ERROR_DOMAIN;

typedef enum {
  OCKAM_TRANSPORT_INTERFACE_ERROR_INVALID_PARAM = 1,
} ockam_error_code_transport_interface_t;

typedef struct ockam_transport ockam_transport_t;

ockam_error_t ockam_transport_connect(ockam_transport_t*  transport,
                                      ockam_reader_t**    reader,
                                      ockam_writer_t**    writer,
                                      ockam_ip_address_t* remote_address,
                                      int16_t  retry_count,   // -1 : forever, 0 : no retries, >0 : number of retries
                                      uint16_t retry_interval // in seconds
);
ockam_error_t ockam_transport_accept(ockam_transport_t*  transport,
                                     ockam_reader_t**    reader,
                                     ockam_writer_t**    writer,
                                     ockam_ip_address_t* remote_address);
ockam_error_t ockam_transport_deinit(ockam_transport_t* transport);
/*
 * tcp socket specific transport
 */
typedef struct ockam_transport_socket_attributes {
  ockam_ip_address_t listen_address;
  ockam_memory_t*    p_memory;
} ockam_transport_socket_attributes_t;

#endif
