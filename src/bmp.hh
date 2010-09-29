/*
** Made by texane <texane@gmail.com>
** 
** Started on Sun Sep 28 11:10:47 2008 texane
** Updated on Mon Oct 27 00:44:19 2008 texane
*/



#ifndef BMP_H_INCLUDED
# define BMP_H_INCLUDED



#include <stdint.h>



typedef struct bmp bmp_t;



struct bmp* bmp_create(void);
void bmp_destroy(struct bmp*);
int bmp_load_file(struct bmp*, const char*);
int bmp_store_file(struct bmp*, const char*);
int bmp_set_format(struct bmp*, uint16_t, uint32_t, uint32_t);
void* bmp_get_data(struct bmp*);
int bmp_get_header(struct bmp*, uint8_t*, uint32_t*);

#if _DEBUG
void bmp_print(const struct bmp*);
#endif /* _DEBUG */



#endif /* ! BMP_H_INCLUDED */
