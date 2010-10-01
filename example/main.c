/*
** Made by fabien le mentec <texane@gmail.com>
** 
** Started on  Fri Oct  1 18:26:50 2010 texane
** Last update Fri Oct  1 20:39:34 2010 texane
*/


#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include "kv.h"


static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;


static void* bar(void* p)
{
  kv_enter('idle');

  size_t i;
  for (i = 0; i < 100; ++i)
  {
    kv_enter('lock');
    pthread_mutex_lock(&lock);
    kv_enter('work');
    usleep(1);
    pthread_mutex_unlock(&lock);
  }

  kv_enter('idle');

  return NULL;
}


int main(int ac, char** av)
{
#define CONFIG_THREAD_COUNT 20
  pthread_t threads[CONFIG_THREAD_COUNT];

  kv_enter('main');

  size_t i;
  for (i = 0; i < CONFIG_THREAD_COUNT; ++i)
    pthread_create(&threads[i], NULL, bar, NULL);

  for (i = 0; i < CONFIG_THREAD_COUNT; ++i)
    pthread_join(threads[i], NULL);

  kv_enter('done');

  return 0;
}
