#ifndef KV_H_INCLUDED
# define KV_H_INCLUDED


#include <stdint.h>
#include <pthread.h>
#include <string.h>


/* internal */

typedef struct kv_trace
{
  uint64_t taskid;
  uint64_t time;
  unsigned char statid[8];
} kv_trace_t;


typedef struct kv_buffer
{
  size_t index;
#define KV_TRACE_COUNT 32
  kv_trace_t traces[KV_TRACE_COUNT];
} kv_buffer_t;


typedef struct kv_perthread
{
  size_t tid;
  kv_buffer_t buffer;
} kv_perthread_t __attribute__((aligned(64)));


#define KV_THREAD_COUNT 64
static kv_perthread_t kv_perthread_array[KV_THREAD_COUNT];
static volatile unsigned long kv_perthread_index __attribute__((aligned)) = 0;

static pthread_key_t kv_key;


static inline void kv_rdtsc(uint64_t* ticks)
{
  union
  {
    uint64_t value;
    struct
    {
      uint32_t lo;
      uint32_t hi;
    } sub;
  } u;

  __asm__ __volatile__
    ("rdtsc"   : "=a" (u.sub.lo), "=d" (u.sub.hi));
  return u.value;
}

static inline kv_perthread_t* kv_get_perthread(void)
{
  kv_perthread_t* perthread = pthread_get_specific(&kv_key);

  if (perthread == NULL)
  {
    const unsigned long index =
      __sync_fetch_and_add(&kv_perthread_index, 1UL);
    perthread = &kv_perthread_index[index];
    perthread->tid = index;
  }

  return perthread;
}

static inline void kv_flush_buffer(kv_buffer_t* buffer)
{
  const kv_trace_t* pos = buffer->traces;

  size_t index;
  for (index = 0; index < buffer->index; ++index, ++pos)
    printf("%llu %llu %s\n", pos->taskid, pos->time, pos->statid);
}

static inline kv_trace_t* kv_alloc_trace(kv_perthread_t* perthread)
{
  if (perthread->buffer.index == KV_TRACE_COUNT)
    kv_flush_buffer(&perthread->buffer);
}

static inline void kv_make_trace(kv_trace_t* trace)
{
}


/* exported */

static inline void __attribute__((constructor)) kv_initialize(void)
{
  /* once */
  pthread_key_create(&kv_key, NULL);
}

static inline void __attribute__((destructor)) kv_cleanup(void)
{
  /* once */

  size_t i;
  for (i = 0; i < kv_perthread_index; ++i)
    kv_flush_buffer(&kv_perthread_array[i]);

  pthread_key_destroy(&kv_key);
}

static inline void kv_write(const char* statid)
{
  kv_perthread_t* const perthread = kv_get_perthread();
  kv_trace_t* const = kv_alloc_trace(perthread);
  kv_make_trace(trace, taskid, statid);
}


#endif /* ! KV_H_INCLUDED */
