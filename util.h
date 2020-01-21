#include "glo.h"

char *concat(const char *s1, const char *s2)
{
  const size_t len1 = strlen(s1);
  const size_t len2 = strlen(s2);
  char *result = malloc(len1 + len2 + 1);
 
  memcpy(result, s1, len1);
  memcpy(result + len1, s2, len2 + 1);
  return result;
}

void parse_URL(char *url, char *hostname, int *port, char *identifier, char *filename)
{
  char protocol[MAX_STR_LEN], scratch[MAX_STR_LEN], *ptr = 0, *nptr = 0;

  strcpy(scratch, url);
  ptr = (char *)strchr(scratch, ':');
  if (!ptr)
  {
    fprintf(stderr, "Wrong url: no protocol specified\n");
    exit(ERROR);
  }

  strcpy(ptr, "\0");
  strcpy(protocol, scratch);

  strcpy(scratch, url);
  ptr = (char *)strstr(scratch, "//");
  if (!ptr)
  {
    fprintf(stderr, "Wrong url: no server specified\n");
    exit(ERROR);
  }
  ptr += 2;

  strcpy(hostname, ptr);
  nptr = (char *)strchr(ptr, ':');
  if (!nptr)
  {
    *port = 80;
    nptr = (char *)strchr(hostname, '/');
  }
  else
  {
    sscanf(nptr, ":%d", port);
    nptr = (char *)strchr(hostname, ':');
  }

  if (nptr)
    *nptr = '\0';

  nptr = (char *)strchr(ptr, '/');

  if (!nptr)
  {
    fprintf(stderr, "Wrong url: no file specified\n");
    exit(ERROR);
  }

  strcpy(identifier, nptr);

  ptr = strrchr(nptr, '/');
  ptr++;
  ptr = concat("file_", ptr);
  strcpy(filename, ptr);
}

void parse_command(int argc, char *argv[], char *url, char *hostname, int *port, char *identifier, char *filename)
{
  int i;
  
  strcpy(url, argv[1]);
  THREAD_COUNT = atoi(argv[2]);

  parse_URL(url, hostname, port, identifier, filename);

  printf("\n-- Hostname = %s , Port = %ls , Identifier = %s\n", hostname, port, identifier);
}

int create_socket(char *hostname, int port)
{

  int sd, rc;
  struct sockaddr_in localAddr, servAddr;
  struct hostent *h;

  h = gethostbyname(hostname);
  if (h == NULL)
  {
    printf("unknown host: %s \n ", hostname);
    exit(ERROR);
  }

  servAddr.sin_family = h->h_addrtype;
  memcpy((char *)&servAddr.sin_addr.s_addr, h->h_addr_list[0], h->h_length);
  servAddr.sin_port = htons(port);

  sd = socket(AF_INET, SOCK_STREAM, 0);
  if (sd < 0)
  {
    perror("cannot open socket ");
    exit(ERROR);
  }

  localAddr.sin_family = AF_INET;
  localAddr.sin_addr.s_addr = htonl(INADDR_ANY);
  localAddr.sin_port = htons(0);

  rc = bind(sd, (struct sockaddr *)&localAddr, sizeof(localAddr));
  if (rc < 0)
  {
    printf("%s: cannot bind port TCP %u\n", "file", port);
    perror("error ");
    exit(ERROR);
  }

  rc = connect(sd, (struct sockaddr *)&servAddr, sizeof(servAddr));
  if (rc < 0)
  {
    perror("cannot connect ");
    exit(ERROR);
  }

  return sd;
}

int get_content_length(char *response)
{
  char tag[MAX_STR_LEN];
  int filesize;
  strcpy(tag, "Content-Length: ");
  sprintf(tag, "%s", strstr(response, tag) + strlen(tag));
  filesize = atoi(tag);
  return filesize;
}

void clprint(int val, int diff)
{
  int oldVal = val - diff;
  char sign;
  if (diff < 0)
  {
    sign = '-';
    diff *= -1;
  }
  else
  {
    sign = '+';
  }
  printf(ANSI_COLOR_CYAN " %d" ANSI_COLOR_YELLOW " %c %d = " ANSI_COLOR_RED "%d\n" ANSI_COLOR_RESET, oldVal, sign, diff, val);
}

int count_digits(int num)
{
  if (num < 0)
  {
    perror("invalid int value");
    exit(ERROR);
  }

  int digits = 0;

  do
    digits++;
  while ((num = num / 10) != 0);

  return digits;
}

char *itoa(int num)
{
  int size = (count_digits(num) + 1) * sizeof(char);
  char *result = malloc(size);
  snprintf(result, size, "%d", num);
  return result;
}
