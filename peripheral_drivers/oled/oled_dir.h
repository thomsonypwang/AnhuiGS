#ifndef __OLED_DIR_H
#define __OLED_DIR_H		

#include "hc32_ll.h"

#define OLED_WIDTH  64
#define OLED_HEIGHT 128

void oled_init(void);
void oled_clear(void);
void oled_display(void);
void oled_paint_clear(void);
void oled_draw_point(uint8_t x, uint8_t y);
void oled_clear_point(uint8_t x, uint8_t y);
void oled_show_char(uint8_t x, uint8_t y, uint8_t chr, uint8_t size1);
void oled_show_string(uint8_t x, uint8_t y, uint8_t *chr, uint8_t size1);
void oled_show_num(uint8_t x, uint8_t y, uint32_t num, uint8_t len, uint8_t size1);
//void oled_show_float_num(uint8_t x,uint8_t y,float Fnum,uint8_t size1,uint8_t mode);
void oled_show_chinese(uint8_t x, uint8_t y, uint8_t num, uint8_t size1);
void oled_scroll_display(uint8_t num, uint8_t space);


#endif  
	 

