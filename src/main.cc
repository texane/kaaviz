//
// Made by fabien le mentec <texane@gmail.com>
// 
// Started on  Wed Sep 29 21:18:01 2010 texane
// Last update Thu Sep 30 05:08:29 2010 fabien le mentec
//



#include <map>
#include <list>
#include <string>

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include "bmp.hh"


// file reading

static char line_buf[1024];

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
  std::string _taskid;
  uint64_t _time;
  bool _isnew;
  std::string _statid;

  struct trace_entry() : _isnew(true) {}

} trace_entry_t;

static int next_word(char*& line, char* word)
{
  for (; *line == ' '; ++line)
    ;

  if (*line == 0)
    return -1;

  word = *line;

  for (; *line && (*line != ' '); ++line)
    ;

  *line = 0;

  return 0;
}

// trace info

typedef struct slice
{
  uint64_t _start;
  uint64_t _stop;
  size_t _state;

  slice() : _start(0), _stop(0) {}
  slice(uint64_t start, size_t state) :
    _start(start), _stop(0), _state(state) {}

} slice_t;

typedef std::list<slice_t> slice_list_t;
typedef std::map<std::string, slice_list_t> slices_map_t;
typedef std::map<std::string, size_t> id_map_t;

typedef struct trace_info
{
  // taskid -> slices
  slices_map_t _slices;
  // taskid -> index
  id_map_t _taskids;
  // statid -> index
  id_map_t _statids;
} trace_info_t;

static inline size_t id_to_index
(const std::string& id, const id_map_t& map)
{
  // assume id a valid key
  return map.find(id)->second;
}

static int next_trace(mapped_file_t* mf, trace_entry_t& te)
{
  char* line;

  if (read_line(mf, &line) == -1)
    return -1;

  // tid
  if (next_word(line, word) == -1)
    return -1;
  te._taskid = std::string(word);

  // time
  if (next_word(line, word) == -1)
    return -1;
  te._time = (uint64_t)strtoul(word);

  // isdone statid
  const char* statid = word;
  if ((*word == '>') || (*word == '<'))
  {
    ++statid;
    if (*word == '<')
      te._isnew = false;
  }

  te._statid = std::string(statid);

  return 0;
}

static int load_trace_file(const char* path, trace_info_t& ti)
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

  trace_entry_t te;
  while ((next_trace(&mf, &te)) != -1)
  {
    // is this a new task
    slices_map_t::iterator smi = ti._slices.find(ti._taskid);
    if (smi == ti._slices.end())
    {
      slices_map_t::value_type keyval =
	std::make_pair(ti._taskid, slice_list_t());
      smi = ti._slices.insert(keyval)->first;
    }

    // is this a new state
    if (ti._statids.find(te._statid) == ti._statids.end())
    {
      id_map_t::value_type keyval =
	std::make_pair(ti._statid, ti._statids.size());
      ti._statids.insert(keyval);
    }

    // slice list for _taskid
    slice_list_t& sl = smi->second;

    // close the last slice if needed
    if (sl.size())
    {
      slice_list_t::iterator& last = sl.back();
      if (last->_stop <= last->_start)
	last->_stop = te._time;
    }

    // insert new slice if needed
    if (te._isnew == true)
      sl.push_back(slice(id_to_index(te._statid), te._time));
  }

  // foreach task, close the last slice
  slices_map_t::iterator pos = ti._slices.begin();
  slices_map_t::iterator end = ti._slices.end();
  for (; pos != end; ++pos)
  {
    if (sl.size() == 0)
      continue ;

    slice_list_t::iterator& last = sl.back();
    if (last->_stop <= last->_start)
      last->_stop = ti._maxtime;
  }

  error = 0;

 on_error:
  unmap_file(&mf);

  return error;
}

#if 0 // todo
static void draw_rect
(bmp_t& bmp, uint32_t x, uint32_t y, uint32_t w, uint32_t h)
{
}
#endif

static int output_slices(const char* path, const trace_info_t& ti)
{
  // state color step
  const size_t scs = 255 / ti._states.size();

  // heigth unit size
  const size_t hus = img.heigth / ti._tasks.size();

  // width unit size
  const size_t wus = img.width / (ti._max_time - ti._min_time);

  // foreach slice list
  slices_map_t::const_iterator smi = ti._slices.begin();
  slices_map_t::const_iterator sme = ti._slices.end();
  for (; smi != sme; ++smi)
  {
    const size_t tid = id_to_index(smi->first, ti._taskids);

    // foreach slice
    slices_list_t::const_iterator sli = smi->first->begin();
    slices_list_t::const_iterator sle = smi->first->end();
    for (; sli != sle; ++sli)
    {
#if 0 // todo
      const unsigned int rgb = id_to_index(si->_statid, ti._statid_map) * scs;
      const size_t rw = (si->_stop - si->_start) * wus;
      draw_rect(si._time, si->_start * wus, tid * hus, wus, rws, rgb);
#endif
    }
  }

  return 0;
}


int main(int ac, char** av)
{
  trace_info_t ti;

  if (load_trace_file("../dat/0.kv", ti) == -1)
    return -1;

  output_slices("/tmp/foo.bmp", ti);

  return 0;
}
