#include <limits.h> 
#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <time.h>

#include <sys/stat.h>

#include "util.h"
#include "request.h"

typedef struct
{
  char *hostname;
  char *identifier;
  int port;
  int count;
} thread_param_t;

pthread_mutex_t read_byte_size_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t downloaded_file_mutex = PTHREAD_MUTEX_INITIALIZER;

void printResponse(char *buffer, int rc, int sd, int base)
{
  int size = rc - (strstr(buffer, "\r\n\r\n") - buffer + 4);

  int remaining = CONTENT_LENGTH - size;

  pthread_mutex_lock(&read_byte_size_mutex);

  fseek(DOWNLOADED_FILE, base, SEEK_SET);
  fwrite(&buffer[strstr(buffer, "\r\n\r\n") - buffer + 4], 1, size, DOWNLOADED_FILE);
  pthread_mutex_unlock(&read_byte_size_mutex);

  rc = 0;

  do
  {
    pthread_mutex_lock(&read_byte_size_mutex);
    memset(buffer, 0x0, BUFFER_SIZE);
    if (remaining < BUFFER_SIZE)
    {
      rc = read(sd, buffer, remaining);
    }
    else
    {
      rc = read(sd, buffer, BUFFER_SIZE);
    }

    if (rc > 0)
    {
      fseek(DOWNLOADED_FILE, base + size, SEEK_SET);
      fwrite(buffer, 1, rc, DOWNLOADED_FILE);

      size += rc;
      remaining -= rc;
      printf("  %8d  %8d %8d\n", size, rc, remaining);
    }
    pthread_mutex_unlock(&read_byte_size_mutex);

  } while ((rc > 0) && (remaining > 0));

  printf("\n   Total recieved response bytes: %d\n", size);
}

char *DOWNLOAD_PATH = "./";

void *thread_function(void *arg)
{

  thread_param_t *thread_param = (thread_param_t *)arg;

  int thread_size = CONTENT_LENGTH / THREAD_COUNT;
  if (thread_param->count == THREAD_COUNT - 1)
  {
    thread_size += CONTENT_LENGTH % THREAD_COUNT;
  }

  int rc;
  int sd = create_socket(thread_param->hostname, thread_param->port);

  pthread_mutex_lock(&read_byte_size_mutex);

  int base = READ_BYTE_SIZE;
  int limit = (REMAINING_BYTE_SIZE < thread_size) ? REMAINING_BYTE_SIZE : thread_size;
  range_request(thread_param->hostname, thread_param->identifier, base, limit, sd, thread_param->count);

  printf("\n-- Thread[%d]", thread_param->count);
  printf(" downloading %d bytes\n", thread_size);

  printf("---TOTAL READ BYTE SIZE :");
  READ_BYTE_SIZE += thread_size;
  clprint(READ_BYTE_SIZE, thread_size);

  printf("---TOTAL REMAINING BYTE SIZE :");
  REMAINING_BYTE_SIZE -= thread_size;
  clprint(REMAINING_BYTE_SIZE, -thread_size);

  pthread_mutex_unlock(&read_byte_size_mutex);

  char buffer[BUFFER_SIZE];
  memset(buffer, 0x0, BUFFER_SIZE);
  if ((rc = read(sd, buffer, BUFFER_SIZE)) < 0)
  {
    perror("read");
    exit(1);
  }
  printResponse(buffer, rc, sd, base);

  pthread_exit(0);
}

int start_threads(char *hostname, char *identifier, int port)
{
  int cnt;
  pthread_t p_thread[THREAD_COUNT];

  for (cnt = 0; cnt < THREAD_COUNT; cnt++)
  {
    thread_param_t *thread_param = malloc(sizeof(thread_param_t *));

    thread_param->hostname = hostname;
    thread_param->identifier = identifier;
    thread_param->port = port;
    thread_param->count = cnt;

    if (pthread_create(&p_thread[cnt], NULL, thread_function, (void *)thread_param))
    {
      fprintf(stderr, "Error creating the thread");
    }
  }

  for (cnt = 0; cnt < THREAD_COUNT; cnt++)
  {
    pthread_join(p_thread[cnt], NULL);
  }

  fprintf(stdout, "\nAll threads completed.\n");
  return 0;
}

int main(int argc, char *argv[])
{
  char url[MAX_STR_LEN];
  char hostname[MAX_STR_LEN];
  int port;
  char identifier[MAX_STR_LEN];
  char filename[MAX_STR_LEN];

  int rc, i;

  char line[MAX_MSG];
  int size;

  parse_command(argc, argv, url, hostname, &port, identifier, filename);

  struct stat st = {0};
  if (stat(DOWNLOAD_PATH, &st) == -1)
  {
    mkdir(DOWNLOAD_PATH, 0700);
  }

  DOWNLOAD_PATH = concat(DOWNLOAD_PATH, filename);

  DOWNLOADED_FILE = fopen(DOWNLOAD_PATH, "w+b");

  int sd = create_socket(hostname, port);

  standart_request(hostname, identifier, sd);

  char buffer[BUFFER_SIZE];
  memset(buffer, 0x0, BUFFER_SIZE); 
  if ((rc = read(sd, buffer, BUFFER_SIZE)) < 0)
  {
    perror("read");
    exit(1);
  }

  CONTENT_LENGTH = get_content_length(buffer);


  READ_BYTE_SIZE = 0;
  REMAINING_BYTE_SIZE = CONTENT_LENGTH;

  start_threads(hostname, identifier, port);

  fclose(DOWNLOADED_FILE);

  close(sd);

  char buf[PATH_MAX + 1]; 
  char *res = realpath(DOWNLOAD_PATH, buf);
  if (res)
  {
    printf("You can find your file at :\n");
    printf("\n-- " ANSI_COLOR_YELLOW "%s\n\n" ANSI_COLOR_RESET, buf);
  }
  else
  {
    perror("realpath");
    exit(EXIT_FAILURE);
  }

  return SUCCESS;
}

