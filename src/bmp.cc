/*
** Made by texane <texane@gmail.com>
** 
** Started on Sun Sep 28 11:15:46 2008 texane
** Updated on Mon Oct 27 02:21:12 2008 texane
*/



#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include "bmp.h"



/* file header
 */

struct __attribute__((packed)) file_header
{
  /* bmp header
   */

  uint16_t magic;
  uint32_t file_size;
  uint16_t reserved[2];
  uint32_t data_offset;

  /* dib header
   */
  
  uint32_t dib_size;
  int32_t width;
  int32_t height;
  uint16_t color_plane_count;
  uint16_t bpp;
  uint32_t compression;
  uint32_t data_size;
  int32_t horizontal_resolution;
  int32_t vertical_resolution;
  uint32_t palette_color_count;
  uint32_t palette_important_count;

  /* palette
   */
};



/* bmp type
 */

struct bmp
{
  uint16_t bpp;
  uint32_t width;
  uint32_t height;
  uint8_t* data;

#define BMP_FLAG_HAS_FORMAT 0
  uint8_t flags;
};



/* check bmp info
 */

static int check_format(uint16_t bpp, int32_t width, int32_t height)
{
  int64_t n;

  if (bpp != 24)
    return -1;

  if (width < 0 || height < 0)
    return -1;

  n = (int64_t)width * height;

  if (n > INT32_MAX)
    return -1;

  if ((n * (bpp / 8)) > INT32_MAX)
    return -1;

  return 0;
}


static int check_file_header(const struct file_header* hdr, size_t file_size)
{
#define BMP_MAGIC 0x4d42
  if (hdr->magic != BMP_MAGIC)
    return -1;

  if (check_format(hdr->bpp, hdr->width, hdr->height) == -1)
    return -1;

  if (hdr->file_size != file_size)
    return -1;

#define DIB_SIZE 0x28
  if (hdr->dib_size != DIB_SIZE)
    return -1;

  if (hdr->color_plane_count != 1)
    return -1;

  if (hdr->compression != 0)
    return -1;

  if (hdr->data_size != (hdr->bpp * hdr->width * hdr->height))
    return -1;

  return 0;
}



/* flags
 */

static inline void set_flag(struct bmp* bmp, uint8_t flag)
{
  bmp->flags |= 1 << flag;
}

static inline int test_flag(const struct bmp* bmp, uint8_t flag)
{
  return bmp->flags & (1 << flag);
}



/* io helpers
 */

static int io_n(int fd, ssize_t (*f)(int, void*, size_t), void* buf, size_t size)
{
  ssize_t n;

  while (size)
    {
      n = f(fd, buf, size);

      if ((n == 0) || (n == -1 && errno != EINTR))
	break;

      buf = (uint8_t*)buf + n;

      size -= n;
    }

  return size ? -1 : 0;
}


static int read_n(int fd, void* buf, size_t n)
{
  return io_n(fd, read, buf, n);
}


static int write_n(int fd, const void* buf, size_t n)
{
  return io_n(fd, (void*)write, (void*)buf, n);
}



/* exported
 */

struct bmp* bmp_create(void)
{
  struct bmp* bmp;

  bmp = malloc(sizeof(struct bmp));
  if (bmp == NULL)
    return NULL;

  bmp->bpp = 0; 
  bmp->width = 0; 
  bmp->height = 0; 

  bmp->flags = 0;

  bmp->data = NULL;

  return bmp;
}


void bmp_destroy(struct bmp* bmp)
{
  if (bmp->data != NULL)
    free(bmp->data);

  free(bmp);
}


int bmp_load_file(struct bmp* bmp,
		  const char* filename)
{
  uint8_t* data = NULL;
  struct stat st;
  uint32_t size;
  int res;
  int fd;
  struct file_header hdr;

  res = -1;
  fd = -1;

  if (test_flag(bmp, BMP_FLAG_HAS_FORMAT))
    goto on_error;

  if ((fd = open(filename, O_RDONLY)) == -1)
    goto on_error;

  if (fstat(fd, &st) == -1)
    goto on_error;

  if (st.st_size < sizeof(struct file_header))
    goto on_error;
  
  if (read_n(fd, &hdr, sizeof(struct file_header)) == -1)
    goto on_error;

  if (check_file_header(&hdr, st.st_size) == -1)
    goto on_error;

  size = hdr.bpp * hdr.width * hdr.height;

  if ((data = malloc(size)) == NULL)
    goto on_error;

  if (read_n(fd, data, size) == -1)
    goto on_error;

  bmp->bpp = hdr.bpp;
  bmp->width = hdr.width;
  bmp->height = hdr.height;
  bmp->data = data;

  set_flag(bmp, BMP_FLAG_HAS_FORMAT);

  data = NULL;

  res = 0;

 on_error:

  if (data != NULL)
    free(data);

  if (fd != -1)
    close(fd);

  return res;
}


int bmp_store_file(struct bmp* bmp,
		   const char* filename)
{
  int res;
  int fd;
  void* data;
  uint32_t data_size;
  struct file_header hdr;

  res = -1;

  fd = open(filename, O_WRONLY | O_TRUNC | O_CREAT, 0700);
  if (fd == -1)
    goto on_error;

  data = bmp_get_data(bmp);
  if (data == NULL)
    goto on_error;

  data_size = bmp->bpp * bmp->width * bmp->height;

  memset(&hdr, 0, sizeof(struct file_header));

  hdr.magic = BMP_MAGIC;
  hdr.file_size = sizeof(struct file_header) + data_size;
  hdr.data_offset = sizeof(struct file_header);
  hdr.dib_size = DIB_SIZE;
  hdr.width = (int32_t)bmp->width;
  hdr.height = (int32_t)bmp->height;
  hdr.color_plane_count = 1;
  hdr.bpp = bmp->bpp;
  hdr.data_size = data_size;

  if (write_n(fd, &hdr, sizeof(struct file_header)) == -1)
    goto on_error;

  if (write_n(fd, data, data_size) == -1)
    goto on_error;

  res = 0;

 on_error:

  if (fd != -1)
    close(fd);

  return res;
}


int bmp_set_format(struct bmp* bmp,
		   uint16_t bpp,
		   uint32_t width,
		   uint32_t height)
{
  if (check_format(bpp, width, height) == -1)
    return -1;

  bmp->bpp = bpp;
  bmp->width = width;
  bmp->height = height;

  set_flag(bmp, BMP_FLAG_HAS_FORMAT);

  return 0;
}


void* bmp_get_data(struct bmp* bmp)
{
  if (!test_flag(bmp, BMP_FLAG_HAS_FORMAT))
    return NULL;

  if (bmp->data == NULL)
    bmp->data = malloc(bmp->bpp * bmp->width * bmp->height);

  return bmp->data;
}


int bmp_get_header(struct bmp* bmp, uint8_t* buf, uint32_t* size)
{
  uint32_t data_size;
  struct file_header* hdr;

  if (*size < sizeof(struct file_header))
    return -1;

  data_size = bmp->bpp * bmp->width * bmp->height;
  if (!data_size)
    return -1;

  hdr = (struct file_header*)buf;

  memset(hdr, 0, sizeof(struct file_header));

  hdr->magic = BMP_MAGIC;
  hdr->file_size = sizeof(struct file_header) + data_size;
  hdr->data_offset = sizeof(struct file_header);
  hdr->dib_size = DIB_SIZE;
  hdr->width = (int32_t)bmp->width;
  hdr->height = (int32_t)bmp->height;
  hdr->color_plane_count = 1;
  hdr->bpp = bmp->bpp;
  hdr->data_size = data_size;

  *size = sizeof(struct file_header);

  return 0;
}


#if _DEBUG

# include <stdio.h>

void bmp_print(const struct bmp* bmp)
{
  printf("bmp\n");
  printf(" bpp   : %d\n", bmp->bpp);
  printf(" height: %d\n", bmp->height);
  printf(" width : %d\n", bmp->width);
}

#endif /* _DEBUG */
