#ifndef KV_H_INCLUDED
# define KV_H_INCLUDED



#include <stdint.h>
#include <pthread.h>
#include <string.h>
#include <stdio.h>



/* -Wno-multichar to use multichar constant
 */


/* compile time configuration
 */

#define KV_CONFIG_FLUSH_STDOUT 1
#define KV_CONFIG_FLUSH_MEMORY 0
#define KV_CONFIG_TRACE_COUNT 1024 /* must be pow2 */
#define KV_CONFIG_THREAD_COUNT 64


/* internal
 */

typedef struct kv_trace
{
  uint64_t time;
  uint32_t statid;
} kv_trace_t;


typedef struct kv_buffer
{
  size_t head;
  size_t tail;
  kv_trace_t traces[KV_CONFIG_TRACE_COUNT];
} kv_buffer_t;


typedef struct kv_perthread
{
  size_t tid;
  kv_buffer_t buffer;
} kv_perthread_t __attribute__((aligned(64)));


static kv_perthread_t kv_perthread_array[KV_CONFIG_THREAD_COUNT];
static volatile unsigned long kv_perthread_index __attribute__((aligned)) = 0;

static pthread_key_t kv_key;


static inline uint64_t kv_rdtsc(void)
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
  kv_perthread_t* perthread = pthread_getspecific(kv_key);

  if (perthread == NULL)
  {
    const unsigned long index =
      __sync_fetch_and_add(&kv_perthread_index, 1UL);

    perthread = &kv_perthread_array[index];
    pthread_setspecific(kv_key, (void*)perthread);

    perthread->buffer.head = 0;
    perthread->buffer.tail = 0;
    perthread->tid = index;
  }

  return perthread;
}

static inline void kv_flush_buffer(kv_perthread_t* perthread)
{
  kv_buffer_t* const buffer = &perthread->buffer;

  /* capture the [head, tail[ range */
  size_t head = buffer->head;
  const size_t tail = buffer->tail;

  /* reset by setting head to tail */
  buffer->head = buffer->tail;

  while (head != tail)
  {
    kv_trace_t* pos = &buffer->traces[head];
    printf("%u %llu %x\n", perthread->tid, pos->time, pos->statid);
    head = (head + 1) & (KV_CONFIG_TRACE_COUNT - 1);
  }
}

static inline kv_trace_t* kv_alloc_trace(kv_perthread_t* perthread)
{
  kv_trace_t* const trace = &perthread->buffer.traces[perthread->buffer.tail];
  perthread->buffer.tail =
    (perthread->buffer.tail + 1) & (KV_CONFIG_TRACE_COUNT - 1);
  return trace;
}

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

  pthread_key_delete(kv_key);
}


/* exported
 */

static inline void kv_enter(uint32_t statid)
{
  /* get the time first */
  const uint64_t time = kv_rdtsc();

  kv_perthread_t* const perthread = kv_get_perthread();

  kv_trace_t* const trace = kv_alloc_trace(perthread);
  trace->time = time;
  trace->statid = statid;
}

static inline void kv_leave(uint32_t statid)
{
  /* get the time first */
  const uint64_t time = kv_rdtsc();

  kv_perthread_t* const perthread = kv_get_perthread();

  kv_trace_t* const trace = kv_alloc_trace(perthread);
  trace->time = time;
  trace->statid = statid;
}

static inline void kv_flush(void)
{
  kv_flush_buffer(kv_get_perthread());
}


#endif /* ! KV_H_INCLUDED */
