#include "ockam/io.h"
#include "ockam/transport.h"
#include "ockam/transport/impl.h"
#include "ockam/memory.h"

const char* const OCKAM_TRANSPORT_INTERFACE_ERROR_DOMAIN = "OCKAM_TRANSPORT_INTERFACE_ERROR_DOMAIN";

static ockam_error_t ockam_transport_interface_error_none = {
  OCKAM_ERROR_NONE,
  OCKAM_TRANSPORT_INTERFACE_ERROR_DOMAIN
};

ockam_memory_t* gp_ockam_transport_memory = NULL;

ockam_error_t ockam_transport_connect(ockam_transport_t*  transport,
                                      ockam_reader_t**    reader,
                                      ockam_writer_t**    writer,
                                      ockam_ip_address_t* remote_address,
                                      int16_t             retry_count,
                                      uint16_t            retry_interval)
{
  ockam_error_t error = ockam_transport_interface_error_none;

  if ((transport == 0) || (transport->vtable == 0)) {
    error.code = OCKAM_TRANSPORT_INTERFACE_ERROR_INVALID_PARAM;
    goto exit;
  }

  error = transport->vtable->connect(transport->ctx, reader, writer, remote_address, retry_count, retry_interval);

exit:
  return error;
}
ockam_error_t ockam_transport_accept(ockam_transport_t*  transport,
                                     ockam_reader_t**    reader,
                                     ockam_writer_t**    writer,
                                     ockam_ip_address_t* remote_address)
{
  ockam_error_t error = ockam_transport_interface_error_none;

  if ((transport == 0) || (transport->vtable == 0)) {
    error.code = OCKAM_TRANSPORT_INTERFACE_ERROR_INVALID_PARAM;
    goto exit;
  }

  error = transport->vtable->accept(transport->ctx, reader, writer, remote_address);

exit:
  return error;
}

ockam_error_t ockam_transport_deinit(ockam_transport_t* transport)
{ockam_error_t error = ockam_transport_interface_error_none;

  if ((transport == 0) || (transport->vtable == 0)) {
    error.code = OCKAM_TRANSPORT_INTERFACE_ERROR_INVALID_PARAM;
    goto exit;
  }

  error = transport->vtable->deinit(transport);

exit:
  return error;
}
