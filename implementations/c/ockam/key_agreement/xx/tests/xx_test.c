#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/wait.h>
#include <getopt.h>

#include "ockam/error.h"
#include "ockam/key_agreement.h"
#include "ockam/memory.h"
#include "ockam/memory/stdlib.h"
#include "ockam/log.h"
#include "ockam/transport.h"

#include "ockam/random/urandom.h"
#include "ockam/vault/default.h"

#include "init_vault.h"

#include "xx_test.h"

#define ACK      "ACK"
#define ACK_SIZE 3
#define OK       "OK"
#define OK_SIZE  2

bool               scripted_xx = false;
bool               run_client  = true;
bool               run_server  = true;
uint8_t            vault_opt   = VAULT_OPT_DEFAULT;
ockam_ip_address_t ockam_ip    = { "", "127.0.0.1", 8000 };

void usage()
{
  printf("OPTIONS\n");
  printf("  -a<xxx.xxx.xxx.xxx>\t\tIP Address\n");
  printf("  -p<portnum>\t\t\tPort\n");
  printf("  --no-client \t\tDo not run initiator\n");
  printf("  --no-server \t\tDo not run responder\n");
  printf("  -s \t\t\t\tUse scripted test case\n\n");
  printf("  -v<1:2> \t\t\t\tVault: 1 - Default, 2 - ATECC608A\n\n");
}

ockam_error_t parse_opts(int argc, char* argv[])
{
    static int no_client = 0;
    static int no_server = 0;

    static struct option long_options[] = {
            /* These options set a flag. */
            {"no-client",   no_argument,       &no_client, 1},
            {"no-server",   no_argument,       &no_server, 1},
            {0, 0, 0, 0}
    };

    int option_index = 0;
  int           ch;
  ockam_error_t status = OCKAM_ERROR_NONE;
  while ((ch = getopt_long(argc, argv, "hsa:p:v:", long_options, &option_index)) != -1) {
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

    run_client = no_client == 0;
    run_server = no_server == 0;

  return status;
}

int main(int argc, char* argv[])
{
  ockam_error_t  error             = OCKAM_ERROR_NONE;
  ockam_vault_t  vault             = { 0 };
  ockam_memory_t memory            = { 0 };
  ockam_random_t random            = { 0 };

  /*-------------------------------------------------------------------------
   * Parse options
   *-----------------------------------------------------------------------*/

  error = parse_opts(argc, argv);
  if (error) goto exit;
  ockam_log_info("Address   : %s", ockam_ip.ip_address);
  ockam_log_info("Port      : %u", ockam_ip.port);
  ockam_log_info("Initiator : %d", run_client);
  ockam_log_info("Responder : %d", run_server);
  ockam_log_info("Vault     : %d", vault_opt);
  ockam_log_info("Scripted  : %d", scripted_xx);

  error = ockam_memory_stdlib_init(&memory);
  if (error) goto exit;
    ockam_log_info("Memory init success");

  error = ockam_random_urandom_init(&random);
  if (error) goto exit;
    ockam_log_info("Random init success");

  error = init_vault(&vault, vault_opt, &memory, &random);
  if (error) goto exit;
    ockam_log_info("Vault init success");

  //  error = xx_test_responder(&vault, &memory, &ockam_ip);
  //  error = xx_test_initiator(&vault, &ockam_ip);
  //  goto exit;

  bool require_fork = run_client && run_server;

    bool is_child = false;
  if (require_fork) {
      int32_t responder_process = fork();
      if (responder_process < 0) {
          error = KEYAGREEMENT_ERROR_TEST;
          goto exit;
      }
      is_child = 0 == responder_process;
  }

    if (run_client && (is_child || !require_fork)) {
        ockam_log_info("Starting initiator");
      error = xx_test_initiator(&vault, &memory, &ockam_ip);
      if (error) {
          error = KEYAGREEMENT_ERROR_FAIL;
        goto exit;
      }
        ockam_log_info("Initiator finished successfully");
    }
    if (run_server && (!is_child)) {
        ockam_log_info("Starting responder");
      // This is the server process
      error = xx_test_responder(&vault, &memory, &ockam_ip);
      if (error) goto exit;
        ockam_log_info("Initiator finished successfully");
    }

    if (require_fork && !is_child) {
        // Get exit status from responder_process
        int            fork_status       = 0;
        wait(&fork_status);
        int32_t responder_status = WEXITSTATUS(fork_status);
        if (responder_status) {
            error = KEYAGREEMENT_ERROR_FAIL;
            goto exit;
        }
    }

exit:
  ockam_log_info("Test done");
  if (error) {
      ockam_log_error("%x", error);
  }
  else {
      ockam_log_info("Test succeeded");
  }

  return error;
}
