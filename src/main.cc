//
// Made by fabien le mentec <texane@gmail.com>
// 
// Started on  Wed Sep 29 21:18:01 2010 texane
// Last update Wed Sep 29 22:47:26 2010 texane
//



#include <map>
#include <list>
#include <string>

#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include "bmp.hh"


// file reading

static char line_buf[4096 * 4];

typedef struct mapped_file
{
  unsigned char* base;
  size_t off;
  size_t len;
} mapped_file_t;

static int map_file(mapped_file_t* mf, const char* path)
{
  int error = -1;
  struct stat st;

  const int fd = open(path, O_RDONLY);
  if (fd == -1)
    return -1;

  if (fstat(fd, &st) == -1)
    goto on_error;

  mf->base = mmap(NULL, st.st_size, PROT_READ, MAP_SHARED, fd, 0);
  if (mf->base == MAP_FAILED)
    goto on_error;

  mf->off = 0;
  mf->len = st.st_size;

  error = 0;

 on_error:
  close(fd);

  return error;
}

static void unmap_file(mapped_file_t* mf)
{
  munmap(mf->base, mf->len);
  mf->base = MAP_FAILED;
  mf->len = 0;
}

static int read_line(mapped_file_t* mf, char** line)
{
  const unsigned char* end = mf->base + mf->len;
  const unsigned char* const base = mf->base + mf->off;
  const unsigned char* p;
  size_t skipnl = 0;
  char* s;

  *line = line_buf;

  for (p = base, s = line_buf; p != end; ++p, ++s)
  {
    if (*p == '\n')
    {
      skipnl = 1;
      break;
    }

    *s = (char)*p;
  }

  *s = 0;

  if (p == base)
    return -1;

  mf->off += (p - base) + skipnl;

  return 0;
}


// trace list

typedef struct trace_entry
{
  std::string _tid;
  uint64_t _time;
  std::string _state;
  bool _isdone;

  struct trace_entry() : _isdone(false) {}

} trace_entry_t;

static int next_trace
(const unsigned char*& buf, size_t& size, trace_entry_t& te)
{
}


// trace info

typedef struct trace_info
{
  std::map< std::pair<std::string, std::string> > _slices;
  std::list<std::string> _states;
} trace_info_t;

typedef slice
{
};

typedef struct task_slices
{
  std::string _tid;
  std::list<slice_t> ;
};

static bool load_trace_file
(const unsigned char*& buf, size_t size, std::list<task_slices_t>& tsl)
{
  mapped_file_t mf;
  int error = -1;
  size_t size1;
  size_t size2;
  size_t j = 0;
  size_t i = 0;
  char* line = NULL;

  *m = NULL;

  if (map_file(&mf, path) == -1)
    return -1;

  while ((read_line(&mf, &line)) != -1)
  {
  }

  error = 0;

 on_error:
  unmap_file(&mf);

  return error;
}


static void draw_rect
(bmp_t& bmp, uint32_t x, uint32_t y, uint32_t w, uint32_t h)
{
}


static void output_slices
(bmp_t& bmp, const trace_info_t& ti)
{
  // state color step
  const size_t scs = 255 / ti._states.size();

  // heigth unit size
  const size_t hus = img.heigth / ti._tasks.size();

  // width unit size
  const size_t wus = img.width / (ti._max_time - ti._min_time);

  std::map<task_slices_t>::const_iterator tsi = ti._slices.begin();
  std::map<task_slices_t>::const_iterator tse = ti._slices.end();
  size_t tid = 0;

  // foreach task slice list
  for (; tsi != tse; ++tsi, ++tid)
  {
    // foreach slice
    for (; si != se; ++si)
    {
      const unsigned int rgb = si->_state * scs;
      const size_t rw = (si->_stop - si->_start) * wus;
      draw_rect(si._time, si->_start * wus, tid * hus, wus, rws, rgb);
    }
  }
}


int main(int ac, char** av)
{
  trace_info_t ti;

  load_trace_file(ti);
  output_slices(bmp, ti);

  return 0;
}
