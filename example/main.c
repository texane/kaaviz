/*
** Made by fabien le mentec <texane@gmail.com>
** 
** Started on  Fri Oct  1 18:26:50 2010 texane
** Last update Fri Oct  1 21:37:27 2010 texane
*/


#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include "kv.h"


static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_barrier_t barrier;


static void* bar(void* p)
{
  pthread_barrier_wait(&barrier);

  kv_enter('>sta');

  size_t i;
  for (i = 0; i < 100; ++i)
  {
    kv_enter('>loc');
    pthread_mutex_lock(&lock);
    kv_enter('<loc');

    kv_enter('>wor');
    unsigned int j;
    for (j = 0; j < 100; ++j)
      __asm__ __volatile__ (""::"m"(j));
    kv_enter('<wor');

    pthread_mutex_unlock(&lock);
  }

  return NULL;
}


int main(int ac, char** av)
{
#define CONFIG_THREAD_COUNT 50
  pthread_t threads[CONFIG_THREAD_COUNT];

  size_t i;

  pthread_barrier_init(&barrier, NULL, CONFIG_THREAD_COUNT);

/*   kv_enter('>cre'); */
  for (i = 0; i < CONFIG_THREAD_COUNT; ++i)
    pthread_create(&threads[i], NULL, bar, NULL);
/*   kv_enter('<cre'); */

/*   kv_enter('>joi'); */
  for (i = 0; i < CONFIG_THREAD_COUNT; ++i)
    pthread_join(threads[i], NULL);
/*   kv_enter('<joi'); */

  return 0;
}
