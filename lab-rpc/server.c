/**
 * Server binary.
 */

#include "kv_store.h"
#include <glib.h>
#include <memory.h>
#include <netinet/in.h>
#include <rpc/pmap_clnt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

#ifndef SIG_PF
#define SIG_PF void (*)(int)
#endif

/* TODO: Add global state. */

extern void kvstore_1(struct svc_req *, SVCXPRT *);
GHashTable *ht;

/* Set up and run RPC server. */
int main(int argc, char **argv) {
  register SVCXPRT *transp;

  pmap_unset(KVSTORE, KVSTORE_V1);

  transp = svcudp_create(RPC_ANYSOCK);
  if (transp == NULL) {
    fprintf(stderr, "%s", "cannot create udp service.");
    exit(1);
  }
  if (!svc_register(transp, KVSTORE, KVSTORE_V1, kvstore_1, IPPROTO_UDP)) {
    fprintf(stderr, "%s", "unable to register (KVSTORE, KVSTORE_V1, udp).");
    exit(1);
  }

  transp = svctcp_create(RPC_ANYSOCK, 0, 0);
  if (transp == NULL) {
    fprintf(stderr, "%s", "cannot create tcp service.");
    exit(1);
  }
  if (!svc_register(transp, KVSTORE, KVSTORE_V1, kvstore_1, IPPROTO_TCP)) {
    fprintf(stderr, "%s", "unable to register (KVSTORE, KVSTORE_V1, tcp).");
    exit(1);
  }

  /* TODO: Initialize state. */
  ht = g_hash_table_new(g_bytes_hash, g_bytes_equal);

  svc_run();
  fprintf(stderr, "%s", "svc_run returned");
  exit(1);
  /* NOTREACHED */
}

/* Example server-side RPC stub. */
int *example_1_svc(int *argp, struct svc_req *rqstp) {
  static int result;

  result = *argp + 1;

  return &result;
}

/* TODO: Add additional RPC stubs. */
char **echo_1_svc(char **argp, struct svc_req *rqstp) {

  static char* result;

  result = strdup(*argp);

  return &result;
}

void * put_1_svc(put_request *argp, struct svc_req *) {
  static put_request result;
  // printf("I AMDE IT HEREEEEE");

  // fflush( stdout );
  GBytes *key = g_bytes_new(argp->key.buf_val, argp->key.buf_len); 
  GBytes *value = g_bytes_new(argp->value.buf_val, argp->value.buf_len);
  g_hash_table_insert(ht, key, value);
  
  result.key.buf_val = strdup(argp->key.buf_val);
  result.key.buf_len = argp->key.buf_len;
  result.value.buf_val = strdup(argp->value.buf_val);
  result.value.buf_len = argp->value.buf_len;
  // printf("KEY VAL, %s\n", result.key.buf_val);
  // printf("KEY LEN, %d\n", result.key.buf_len);

  // printf("VALUE VAL, %s\n", result.value.buf_val);
  // printf("KEY LEN, %d\n", result.value.buf_len);
  return &result;
}


buf * get_1_svc(buf *argp, struct svc_req *) {
  static buf result;

  GBytes *key = g_bytes_new(argp->buf_val, argp->buf_len);
  GBytes *value = g_hash_table_lookup(ht, key);

  g_bytes_unref(key);

  if (value != NULL) {
    long unsigned int len;
    const char *data = g_bytes_get_data(value, &len); /* Sets len = 5. */
    result.buf_len = len;
    result.buf_val = strdup(data);
  } else {
    result.buf_len = 0;
    result.buf_val = NULL;
  }
  return &result;
}

