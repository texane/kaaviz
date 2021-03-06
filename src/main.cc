//
// Made by fabien le mentec <texane@gmail.com>
// 
// Started on  Wed Sep 29 21:18:01 2010 texane
// Last update Fri Oct  1 21:29:25 2010 texane
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

  mf->base = (unsigned char*)mmap
    (NULL, st.st_size, PROT_READ, MAP_SHARED, fd, 0);
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
  mf->base = (unsigned char*)MAP_FAILED;
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

  trace_entry() : _isnew(true) {}

} trace_entry_t;

static int next_word(char*& line, char*& word)
{
  for (; *line == ' '; ++line)
    ;

  if (*line == 0)
    return -1;

  word = line;

  for (; *line && (*line != ' '); ++line)
    ;

  if (*line)
  {
    *line = 0;
    ++line;
  }

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
  // max(times(tasks))
  uint64_t _maxtime;
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
  char* word;

  if (read_line(mf, &line) == -1)
    return -1;

  // tid
  if (next_word(line, word) == -1)
    return -1;
  te._taskid = std::string(word);

  // time
  if (next_word(line, word) == -1)
    return -1;
  te._time = (uint64_t)strtoull(word, NULL, 10);

  // isnew, statid
  if (next_word(line, word) == -1)
    return -1;

  const char* statid = word;
  te._isnew = true;
  if ((*statid == '>') || (*statid == '<'))
  {
    if (*statid == '<')
      te._isnew = false;
    ++statid;
  }

  te._statid = std::string(statid);

  return 0;
}

static int load_trace_file(const char* path, trace_info_t& ti)
{
  mapped_file_t mf = {NULL, 0, 0};

  if (map_file(&mf, path) == -1)
    return -1;

  trace_entry_t te;
  while (next_trace(&mf, te) != -1)
  {
    // is this a new task
    slices_map_t::iterator smi = ti._slices.find(te._taskid);
    if (smi == ti._slices.end())
    {
      // new taskid -> index
      ti._taskids.insert(std::make_pair(te._taskid, ti._taskids.size()));

      // new slice
      slices_map_t::value_type keyval =
	std::make_pair(te._taskid, slice_list_t());
      smi = ti._slices.insert(keyval).first;
    }

    // is this a new state
    if (ti._statids.find(te._statid) == ti._statids.end())
    {
      id_map_t::value_type keyval =
	std::make_pair(te._statid, ti._statids.size());
      ti._statids.insert(keyval);
    }

    // slice list for _taskid
    slice_list_t& sl = smi->second;
    if (sl.size())
    {
      // convert to relative time
      te._time -= sl.front()._start;

      // close the last slice if needed
      slice_t& last = sl.back();
      if (last._stop <= last._start)
	last._stop = te._time;
    }

    // insert new slice if needed
    if (te._isnew == true)
    {
      const size_t index = id_to_index(te._statid, ti._statids);
      sl.push_back(slice_t(te._time, index));
    }
  }

  slices_map_t::iterator pos;
  slices_map_t::iterator end = ti._slices.end();

  // find maxtime
  ti._maxtime = 0;
  for (pos = ti._slices.begin(); pos != end; ++pos)
  {
    // slice list for _taskid
    slice_list_t& sl = pos->second;
    if (sl.size() == 0)
      continue ;

    // adjust, no longer a base
    sl.front()._start = 0;

    // maxtime
    const uint64_t last = std::max(sl.back()._start, sl.front()._stop) + 1;
    const uint64_t diff = last - sl.front()._start;
    if (diff > ti._maxtime)
      ti._maxtime = diff;
  }

  // close last if needed
  for (pos = ti._slices.begin(); pos != end; ++pos)
  {
    // slice list for _taskid
    slice_list_t& sl = pos->second;
    if (sl.size() == 0)
      continue ;

    if (sl.back()._stop <= sl.back()._start)
      sl.back()._stop = ti._maxtime;
  }

  unmap_file(&mf);

  return 0;
}

// color allocator

static void generate_colors(size_t)
{
}

static inline const uint8_t* get_color(size_t index)
{
  static uint8_t rgb[][3] =
  {
    { 0xff, 0x00, 0x00 },
    { 0x00, 0xff, 0x00 },
    { 0x00, 0x00, 0xff },
    { 0x80, 0x80, 0x00 },
    { 0x80, 0x00, 0x80 },
    { 0x00, 0x80, 0x80 },
    { 0x80, 0x00, 0x00 },
    { 0x00, 0x80, 0x00 },
    { 0x00, 0x00, 0x80 }
  };

  return rgb[index];
}

static int output_slices
(const char* path, const trace_info_t& ti)
{
#define IMAGE_WIDTH 1500
#define IMAGE_HEIGHT 300

  // heigth unit size
  const double hus = (double)IMAGE_HEIGHT / (double)ti._taskids.size();
  // width unit size
  const double wus = (double)IMAGE_WIDTH / (double)ti._maxtime;

  slices_map_t::const_iterator smi;
  slices_map_t::const_iterator sme;

  int error = -1;

  uint8_t* data;

  generate_colors(ti._statids.size());

  bmp_t* const bmp = bmp_create();
  if (bmp == NULL)
    return -1;

  if (bmp_set_format(bmp, 24, IMAGE_WIDTH, IMAGE_HEIGHT) == -1)
    goto on_error;

  data = (uint8_t*)bmp_get_data(bmp);
  if (data == NULL)
    goto on_error;

  bmp_clear(bmp);

  // foreach slice list
  smi = ti._slices.begin();
  sme = ti._slices.end();
  for (; smi != sme; ++smi)
  {
    const size_t tid = id_to_index(smi->first, ti._taskids);

    // foreach slice
    slice_list_t::const_iterator sli = smi->second.begin();
    slice_list_t::const_iterator sle = smi->second.end();
    for (; sli != sle; ++sli)
    {
      const uint8_t* const rgb = get_color((size_t)sli->_state);

      const uint32_t x = (uint32_t)((double)sli->_start * wus);
      const uint32_t y = (uint32_t)((double)tid * hus);
      const uint32_t w = (uint32_t)((double)(sli->_stop - sli->_start) * wus);

      bmp_draw_rect(bmp, x, y, w, (uint32_t)hus, rgb);
    }
  }

  if (bmp_store_file(bmp, path) == -1)
    goto on_error;

  error = 0;

 on_error:
  if (bmp != NULL)
    bmp_destroy(bmp);

  return error;
}


int main(int ac, char** av)
{
  trace_info_t ti;

  if (load_trace_file("/tmp/foo.kv", ti) == -1)
    return -1;

  output_slices("/tmp/foo.bmp", ti);

  return 0;
}
