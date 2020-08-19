
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

#include "xx_test.h"

#define ACK      "ACK"
#define ACK_SIZE 3
#define OK       "OK"
#define OK_SIZE  2

bool               scripted_xx = false;
bool               run_client  = true;
bool               run_server  = true;
ockam_ip_address_t ockam_ip    = { "", "127.0.0.1", 8000 };

void usage()
{
  printf("OPTIONS\n");
  printf("  -a<xxx.xxx.xxx.xxx>\t\tIP Address\n");
  printf("  -p<portnum>\t\t\tPort\n");
  printf("  -i \t\t\t\tRun initiator\n");
  printf("  -r \t\t\t\tRun responder\n");
  printf("  -s \t\t\t\tUse scripted test case\n\n");
}

int parse_opts(int argc, char* argv[])
{
  int ch;
  int status = 0;
  while ((ch = getopt(argc, argv, "hsira:p:")) != -1) {
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

    case '?':
      status = -1;
      usage();
      ockam_log_error("invalid command-line arguments: %d", status);
      return 2;

    default:
      break;
    }
  }

  return status;
}

int main(int argc, char* argv[])
{
  ockam_vault_t  vault             = { 0 };
  ockam_memory_t memory            = { 0 };
  ockam_random_t random            = { 0 };
  int            responder_status  = 0;
  int            initiator_status  = 0;
  int            fork_status       = 0;
  int32_t        responder_process = 0;

  ockam_vault_default_attributes_t vault_attributes = { .memory = &memory, .random = &random };

  ockam_error_t error = ockam_memory_stdlib_init(&memory);
  if (ockam_error_has_error(&error)) goto exit;

  error = ockam_random_urandom_init(&random);
  if (ockam_error_has_error(&error)) goto exit;

  error = ockam_vault_default_init(&vault, &vault_attributes);
  if (ockam_error_has_error(&error)) goto exit;

  /*-------------------------------------------------------------------------
   * Parse options
   *-----------------------------------------------------------------------*/
  int status = parse_opts(argc, argv);
  if (status) goto exit;
  printf("Address     : %s\n", ockam_ip.ip_address);
  printf("Port        : %u\n", ockam_ip.port);
  printf("Initiator   : %d\n", run_client);
  printf("Responder   : %d\n", run_server);

  //  error = xx_test_responder(&vault, &memory, &ockam_ip);
  //  error = xx_test_initiator(&vault, &ockam_ip);
  //  goto exit;

  responder_process = fork();
  if (responder_process < 0) {
    status = -1;
    goto exit;
  }
  if (0 != responder_process) {
    if (run_client) {
      error = xx_test_initiator(&vault, &memory, &ockam_ip);
      if (ockam_error_has_error(&error)) {
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
      if (ockam_error_has_error(&error)) goto exit;
    }
  }

exit:
  printf("Tests done\n");
  if (initiator_status) printf("Initiator failed.\n");
  if (responder_status) printf("Responder failed.\n");
  if (ockam_error_has_error(&error)) ockam_log_error("%s: %d", error.domain, error.code);
  if (status) ockam_log_error("Status: %d", status);

  return error.code + status;
}
