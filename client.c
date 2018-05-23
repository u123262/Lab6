#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <getopt.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <sys/types.h>
#include "utils.h"

#define BUF_SIZE 50

struct Server {
  char ip[255];
  int port;
};

bool ConvertStringToUI64(const char *str, uint64_t *val) {
  char *end = NULL;
  unsigned long long i = strtoull(str, &end, 10);
  if (errno == ERANGE) {
    fprintf(stderr, "Out of uint64_t range: %s\n", str);
    return false;
  }

  if (errno != 0)
    return false;

  *val = i;
  return true;
}

int main(int argc, char **argv) {
  uint64_t k = -1;
  uint64_t mod = -1;
  char servers[255] = {'\0'}; // TODO: explain why 255

  while (true) {
    int current_optind = optind ? optind : 1;

    static struct option options[] = {{"k", required_argument, 0, 0},
                                      {"mod", required_argument, 0, 0},
                                      {"servers", required_argument, 0, 0},
                                      {0, 0, 0, 0}};

    int option_index = 0;
    int c = getopt_long(argc, argv, "", options, &option_index);

    if (c == -1)
      break;

    switch (c) {
    case 0: {
      switch (option_index) {
      case 0:
        ConvertStringToUI64(optarg, &k);
        // TODO: your code here
        if (k <= 1) {
          printf("k > 1\n");
          return 1;
        }
        break;
      case 1:
        ConvertStringToUI64(optarg, &mod);
        // TODO: your code here
        if (mod <= 0) {
          printf("mod > 0\n");
          return 1;
        }
        break;
      case 2:
        // TODO: your code here
        memcpy(servers, optarg, strlen(optarg));
        break;
      default:
        printf("Index %d is out of options\n", option_index);
      }
    } break;

    case '?':
      printf("Arguments error\n");
      break;
    default:
      fprintf(stderr, "getopt returned character code 0%o?\n", c);
    }
  }

  if (k == -1 || mod == -1 || !strlen(servers)) {
    fprintf(stderr, "Using: %s --k 1000 --mod 5 --servers /path/to/file\n",
            argv[0]);
    return 1;
  }

  // TODO: for one server here, rewrite with servers from file
  unsigned int servers_num = 0;
  char buf[BUF_SIZE];
  FILE *fp;

  if((fp = fopen(servers, "r")) == NULL) {
      printf("Unable to open file\n");
      return 1;
  }
  while(fgets(buf, BUF_SIZE, fp)){
     servers_num++;
  }
  fseek(fp, 0L, SEEK_SET);
  
  struct Server *to = malloc(sizeof(struct Server) * servers_num);
  servers_num = 0;
  while (fgets(buf, BUF_SIZE, fp) != NULL){
      int i = 0;
      
      while(buf[i] != ':')
      {
        i++;
      }

      memcpy(to[servers_num].ip, buf, sizeof(char)*i);
      to[servers_num].port = atoi(&buf[i + 1]);
      
      printf("%s:%d\n", to[servers_num].ip, to[servers_num].port);
      servers_num++;
  }
  printf("\n");
  fclose(fp);
  
  // TODO: delete this and parallel work between servers
  /*to[0].port = 20001;
  memcpy(to[0].ip, "127.0.0.1", sizeof("127.0.0.1"));*/

  // TODO: work continiously, rewrite to make parallel

  uint64_t start = 1;
  uint64_t server_k = k / servers_num;
  
  int sck[1000];
  for (int i = 0; i < servers_num; i++) {
    struct hostent *hostname = gethostbyname(to[i].ip);
    if (hostname == NULL) {
      fprintf(stderr, "gethostbyname failed with %s\n", to[i].ip);
      exit(1);
    }

    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(to[i].port);
    server.sin_addr.s_addr = *((unsigned long *)hostname->h_addr);

    sck[i] = socket(AF_INET, SOCK_STREAM, 0);
    if (sck[i] < 0) {
      fprintf(stderr, "Socket creation failed!\n");
      exit(1);
    }

    if (connect(sck[i], (struct sockaddr *)&server, sizeof(server)) < 0) {
      fprintf(stderr, "Connection failed\n");
      exit(1);
    }

    // TODO: for one server
    // parallel between servers
    uint64_t begin = start;
    uint64_t end = start + server_k;

    if (end > k) 
    {
      end = k;
    }

    char task[sizeof(uint64_t) * 3];
    memcpy(task, &begin, sizeof(uint64_t));
    memcpy(task + sizeof(uint64_t), &end, sizeof(uint64_t));
    memcpy(task + 2 * sizeof(uint64_t), &mod, sizeof(uint64_t));

    if (send(sck[i], task, sizeof(task), 0) < 0) {
      fprintf(stderr, "Send failed\n");
      exit(1);
    }

    start += server_k + 1;
  }
  
  uint64_t answer = 1;
  uint64_t answer_from_server = 0;

  for (int i = 0; i < servers_num; i++) {
    char response[sizeof(uint64_t)];
    if (recv(sck[i], response, sizeof(response), 0) < 0) {
      fprintf(stderr, "Recieve failed\n");
      exit(1);
    }

    // TODO: from one server
    // unite results
    memcpy(&answer_from_server, response, sizeof(uint64_t));
    
    answer = MultModulo(answer, answer_from_server, mod);

    close(sck[i]);
  }
  printf("answer: %llu\n", answer);
  free(to);

  return 0;
}