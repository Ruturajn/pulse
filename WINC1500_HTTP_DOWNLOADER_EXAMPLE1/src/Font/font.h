/*
 *  font.h
 *  i2c
 *
 *  Created by Michael Köhler on 13.09.18.
 *  Copyright 2018 Skie-Systems. All rights reserved.
 *
 */
#ifndef _font_h_
#define _font_h_


#define ZERO_START_INDEX  16
#define A_START_INDEX     33
#define EQUAL_INDEX       29
#define EXCLAIM_INDEX     1
#define SPACE_INDEX       0
#define COLON_INDEX       26

extern const char ssd1306oled_font[][6];
extern const char special_char[][2];

#endif
