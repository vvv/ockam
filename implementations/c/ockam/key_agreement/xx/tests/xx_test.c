
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/wait.h>

#include "ockam/error.h"
#include "ockam/key_agreement.h"
#include "ockam/memory.h"
#include "ockam/memory/stdlib.h"
#include "ockam/log.h"
#include "ockam/transport.h"

#include "ockam/random/urandom.h"
#include "ockam/vault/default.h"

#include "vault/atecc608a/atecc608a.h"
#include "cryptoauthlib.h"
#include "atca_cfgs.h"
#include "atca_iface.h"
#include "atca_device.h"

#include "xx_test.h"

#define ACK      "ACK"
#define ACK_SIZE 3
#define OK       "OK"
#define OK_SIZE  2

typedef enum {
  VAULT_OPT_NONE = 0,
  VAULT_OPT_DEFAULT,
  VAULT_OPT_ATECC608A
} VAULT_OPT_t;

bool               scripted_xx = false;
bool               run_client  = true;
bool               run_server  = true;
uint8_t            vault_opt   = 0;
ockam_ip_address_t ockam_ip    = { "", "127.0.0.1", 8000 };

ATCAIfaceCfg test_atecc608a_cfg =
{
  .iface_type = ATCA_I2C_IFACE,
  .devtype = ATECC608A,
  .atcai2c.slave_address = 0xC0,
  .atcai2c.bus = 1,
  .atcai2c.baud = 100000,
  .wake_delay = 1500,
  .rx_retries = 20
};

static ockam_vault_atecc608a_io_protection_t test_atecc608a_io_protection =
{                                                   /* IO Protection Key is used to encrypt data sent via */
  .key = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, /* I2C to the ATECC608A. During init the key is       */
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, /* written into the device. In a production system    */
    0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, /* the key should be locked into the device and never */
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37  /* transmitted via I2C.                               */
  },
  .key_size = 32,
  .slot = 6
};

void usage()
{
  printf("OPTIONS\n");
  printf("  -a<xxx.xxx.xxx.xxx>\t\tIP Address\n");
  printf("  -p<portnum>\t\t\tPort\n");
  printf("  -i \t\t\t\tRun initiator\n");
  printf("  -r \t\t\t\tRun responder\n");
  printf("  -s \t\t\t\tUse scripted test case\n\n");
  printf("  -v<1:2> \t\t\t\tVault: 1 - Default, 2 - ATECC608A\n\n");
}

ockam_error_t parse_opts(int argc, char* argv[])
{
  int           ch;
  ockam_error_t status = OCKAM_ERROR_NONE;
  while ((ch = getopt(argc, argv, "hsira:p:v:")) != -1) {
    switch (ch) {
    case 'h':
      usage();
      return 2;

    case 'a':
      strcpy((char*) ockam_ip.ip_address, optarg);
      break;

    case 'p':
      ockam_ip.port = atoi(optarg);
      break;

    case 'i':
      run_client = true;
      break;

    case 'r':
      run_server = true;
      break;

    case 's':
      scripted_xx = true;
      break;

    case 'v':
      vault_opt = atoi(optarg);
      break;

    case '?':
      status = TRANSPORT_ERROR_BAD_PARAMETER;
      usage();
      ockam_log_error("invalid command-line arguments: %x", status);
      return 2;

    default:
      break;
    }
  }

  return status;
}

int main(int argc, char* argv[])
{
  ockam_error_t  error             = OCKAM_ERROR_NONE;
  ockam_vault_t  vault             = { 0 };
  ockam_memory_t memory            = { 0 };
  ockam_random_t random            = { 0 };
  int            responder_status  = 0;
  int            initiator_status  = 0;
  int            fork_status       = 0;
  int32_t        responder_process = 0;

  ockam_vault_default_attributes_t vault_attributes_default = { .memory = &memory, .random = &random };

  ockam_vault_atecc608a_attributes_t vault_attributes_atecc608a =
  {
    .memory         = &memory,
    .mutex          = 0,
    .atca_iface_cfg = &test_atecc608a_cfg,
    .io_protection  = &test_atecc608a_io_protection
  };

  /*-------------------------------------------------------------------------
   * Parse options
   *-----------------------------------------------------------------------*/

  error = parse_opts(argc, argv);
  if (error) goto exit;
  printf("Address   : %s\n", ockam_ip.ip_address);
  printf("Port      : %u\n", ockam_ip.port);
  printf("Initiator : %d\n", run_client);
  printf("Responder : %d\n", run_server);
  printf("Vault     : %d\n", vault_opt);


  error = ockam_memory_stdlib_init(&memory);
  if (error) goto exit;

  error = ockam_random_urandom_init(&random);
  if (error) goto exit;

  if(vault_opt == VAULT_OPT_DEFAULT) {
    error = ockam_vault_default_init(&vault, &vault_attributes_default);
    if (error) goto exit;
  } else if(vault_opt == VAULT_OPT_ATECC608A) {
    error = ockam_vault_atecc608a_init(&vault, &vault_attributes_atecc608a);
    if (error != OCKAM_ERROR_NONE) {
      goto exit;
    }
  }

  //  error = xx_test_responder(&vault, &memory, &ockam_ip);
  //  error = xx_test_initiator(&vault, &ockam_ip);
  //  goto exit;

  responder_process = fork();
  if (responder_process < 0) {
    error = KEYAGREEMENT_ERROR_TEST;
    goto exit;
  }
  if (0 != responder_process) {
    if (run_client) {
      error = xx_test_initiator(&vault, &memory, &ockam_ip);
      if (error) {
        initiator_status = -1;
        goto exit;
      }
    } // end if(run_client)
    // Get exit status from responder_process
    wait(&fork_status);
    responder_status = WEXITSTATUS(fork_status);
    if (responder_status) {
      responder_status = -2;
      goto exit;
    }
  } else {
    if (run_server) {
      // This is the server process
      error = xx_test_responder(&vault, &memory, &ockam_ip);
      if (error) goto exit;
    }
  }

exit:
  printf("Tests done\n");
  if (initiator_status) printf("Initiator failed.\n");
  if (responder_status) printf("Responder failed.\n");
  if (error) ockam_log_error("%x", error);
  return error;
}
