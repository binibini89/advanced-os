#include <stdlib.h>
#include <stdio.h>
#include <rpc/rpc.h>
#include "proxy_rpc.h" 

int main(int argc, char* argv[]){
  CLIENT *cl;
  char** result;
  char* url;
  char* server;
  
if (argc < 3) {
    fprintf(stderr, "usage: %s host url\n", argv[0]);
    exit(1);
  }
  /*
   * Save values of command line arguments
   */
  server = argv[1];
  url = argv[2];

  /*
   * Your code here.  Make a remote procedure call to
   * server, calling httpget_1 with the parameter url
   * Consult the RPC Programming Guide.
   */

  cl = clnt_create(server, HTTPGETPROG, HTTPGETVERS, "tcp");
  if (cl == NULL) {
    clnt_pcreateerror(server);
    exit(1);
  }

  result = httpget_1(&url, cl);
  if (result == NULL) {
    clnt_perror(cl, server);
    exit(1);
  }

  if (*result == 0) {
    fprintf(stderr, "%s: %s couldn't print your message, too bad so sad!\n", argv[0], server);
    exit(1);
  }

  printf("%s\n", *result);
   

  return 0;
}