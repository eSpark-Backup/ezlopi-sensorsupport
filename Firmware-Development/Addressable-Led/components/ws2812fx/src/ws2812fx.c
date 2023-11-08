/* SPDX-License-Identifier: AGPL-3.0-or-later */
/* LED Controller for a matrix of smart LEDs */
/* Copyright (C) 2018-2021 Symonics GmbH, Christian Hoene */

/* #####################################################
 #
 #  Color and Blinken Functions
 #
 ##################################################### */

#include "ws2812fx.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "led.h"
#include "config.h"
#include <freertos/task.h>
#include <palette.h>
#include <cpt_palettes.h>

/*function prototypes*/
static uint16_t random16mm( uint16_t min, uint16_t lim);
static uint16_t random16l( uint16_t lim);
static uint32_t color_blend(uint32_t color1, uint32_t color2, uint8_t blend);

/* A PROGMEM (flash mem) table containing 8-bit unsigned sine wave (0-255).
 Copy & paste this snippet into a Python REPL to regenerate:
 import math
 for x in range(256):
 print("{:3},".format(int((math.sin(x/128.0*math.pi)+1.0)*127.5+0.5))),
 if x&15 == 15: print
 */
static const uint8_t sineTable[256] = {
    128, 131, 134, 137, 140, 143, 146, 149, 152, 155, 158, 162, 165, 167, 170,
    173, 176, 179, 182, 185, 188, 190, 193, 196, 198, 201, 203, 206, 208, 211,
    213, 215, 218, 220, 222, 224, 226, 228, 230, 232, 234, 235, 237, 238, 240,
    241, 243, 244, 245, 246, 248, 249, 250, 250, 251, 252, 253, 253, 254, 254,
    254, 255, 255, 255, 255, 255, 255, 255, 254, 254, 254, 253, 253, 252, 251,
    250, 250, 249, 248, 246, 245, 244, 243, 241, 240, 238, 237, 235, 234, 232,
    230, 228, 226, 224, 222, 220, 218, 215, 213, 211, 208, 206, 203, 201, 198,
    196, 193, 190, 188, 185, 182, 179, 176, 173, 170, 167, 165, 162, 158, 155,
    152, 149, 146, 143, 140, 137, 134, 131, 128, 124, 121, 118, 115, 112, 109,
    106, 103, 100, 97,  93,  90,  88,  85,  82,  79,  76,  73,  70,  67,  65,
    62,  59,  57,  54,  52,  49,  47,  44,  42,  40,  37,  35,  33,  31,  29,
    27,  25,  23,  21,  20,  18,  17,  15,  14,  12,  11,  10,  9,   7,   6,
    5,   5,   4,   3,   2,   2,   1,   1,   1,   0,   0,   0,   0,   0,   0,
    0,   1,   1,   1,   2,   2,   3,   4,   5,   5,   6,   7,   9,   10,  11,
    12,  14,  15,  17,  18,  20,  21,  23,  25,  27,  29,  31,  33,  35,  37,
    40,  42,  44,  47,  49,  52,  54,  57,  59,  62,  65,  67,  70,  73,  76,
    79,  82,  85,  88,  90,  93,  97,  100, 103, 106, 109, 112, 115, 118, 121,
    124};

static inline uint8_t sine8(uint8_t x) {
  return sineTable[x]; // 0-255 in, 0-255 out
}

static uint16_t _rand16seed;

static inline int min(int a, int b) {
  if (a < b)
    return a;
  return b;
}

static inline int max(int a, int b) {
  if (a > b)
    return a;
  return b;
}

// Custom map function for ESP-IDF with uint16_t
static inline uint16_t map(uint16_t value, uint16_t fromLow, uint16_t fromHigh, uint16_t toLow, uint16_t toHigh) {
    // Check for invalid input
    if (value < fromLow) {
        value = fromLow;
    } else if (value > fromHigh) {
        value = fromHigh;
    }

    // Perform the mapping calculation
    return (uint16_t)((uint32_t)(value - fromLow) * (uint32_t)(toHigh - toLow) / (uint32_t)(fromHigh - fromLow) + toLow);
}

// some common colors
// #define GREEN fx_segments[line]->colors[0]
// #define RED fx_segments[line]->colors[1]
// #define BLUE fx_segments[line]->colors[2]

#define GREEN 0x0000FF00
#define RED 0x00FF0000
#define BLUE 0x000000FF

#define WHITE (uint32_t)0xFFFFFF
#define BLACK (uint32_t)0x000000
#define YELLOW (uint32_t)0xFFFF00
#define CYAN (uint32_t)0x00FFFF
#define MAGENTA (uint32_t)0xFF00FF
#define PURPLE (uint32_t)0x400080
#define ORANGE (uint32_t)0xFF3000
#define PINK (uint32_t)0xFF1493
#define ULTRAWHITE (uint32_t)0xFFFFFFFF
#define WARMWHITE (uint32_t)0xffffdf

#define NUM_COLORS 3 /* number of colors per segment */

struct FX_SEGMENT { // 20 bytes
  uint16_t start;
  uint16_t stop;
  uint16_t speed;
  uint8_t mode;
  uint8_t options;
  uint8_t intensity;
  uint8_t palette;

  uint32_t colors[NUM_COLORS];
  uint32_t rgbColor;

  uint32_t next_time;
  uint32_t counter_mode_step;
  uint32_t counter_mode_call;
  uint16_t
      aux_param; // auxilary param (usually stores a WS2812FX_color_wheel index)
  uint16_t aux_param2;  // auxilary param (usually stores bitwise options)
  uint16_t aux_param3; // auxilary param (usually stores a segment index)
};

struct FX_SEGMENT *fx_segments[8];

#define CYCLE (uint8_t)0x40

static inline void setCycle(int line) { fx_segments[line]->aux_param2 |= CYCLE; }

static inline void clearCycle(int line) {
  fx_segments[line]->aux_param2 &= ~CYCLE;
}

static uint16_t segmentLength(int line) {
  return SEGMENT->stop - SEGMENT->start + 1;
}

#define DEFAULT_BRIGHTNESS (uint8_t)50
#define DEFAULT_MODE (uint8_t)0
#define DEFAULT_SPEED (uint16_t)1000

void WS2812FX_init(
    int line, int n, 
    uint32_t colorPrimary, 
    uint32_t colorSecondary, 
    uint32_t colorTertiary,
    uint16_t start,
    uint16_t stop,
    int mode,
    uint8_t intensity,
    uint16_t speed,
    uint8_t options,
    uint8_t palette
) {
  assert(line >= 0 && line <= 16);
  if(SEGMENT == NULL){
    SEGMENT = calloc(1,sizeof(struct FX_SEGMENT));
  }
  // memset(fx_segments + line, 0, sizeof(struct FX_SEGMENT));
  SEGMENT->mode = mode;
  SEGMENT->colors[0] = colorPrimary; //G
  SEGMENT->colors[1] = colorSecondary; //R
  SEGMENT->colors[2] = colorTertiary; //B
  SEGMENT->rgbColor = colorPrimary;
  SEGMENT->start = start;
  SEGMENT->stop = stop;
  SEGMENT->counter_mode_step = 0;
  SEGMENT->counter_mode_call = 0;
  SEGMENT->intensity  = intensity;
  SEGMENT->speed = speed;
  SEGMENT->options = options;
  SEGMENT->palette = palette;
}

void WS2812FX_deinit(int line){
  free(SEGMENT);
  SEGMENT = NULL;
  config_reduce_segment();
}

void WS2812FX_call(int line, uint16_t (*fct)(int)) {
  uint32_t now = xTaskGetTickCount();

  if (!fx_segments[line]->counter_mode_call) {
    uint32_t delay = fct(line);
    fx_segments[line]->next_time = now + delay / portTICK_PERIOD_MS;
    fx_segments[line]->counter_mode_call++;
  } else if (now >= fx_segments[line]->next_time) {
    uint32_t delay = fct(line);
    fx_segments[line]->next_time += delay / portTICK_PERIOD_MS;
    fx_segments[line]->counter_mode_call++;
  }
}

/*
 * Put a value 0 to 255 in to get a color value.
 * The colours are a transition r -> g -> b -> back to r
 * Inspired by the Adafruit examples.
 */
static uint32_t color_wheel(uint8_t pos) {
  pos = 255 - pos;
  if (pos < 85) {
    return ((uint32_t)(255 - pos * 3) << 16) | ((uint32_t)(0) << 8) | (pos * 3);
  } else if (pos < 170) {
    pos -= 85;
    return ((uint32_t)(0) << 16) | ((uint32_t)(pos * 3) << 8) | (255 - pos * 3);
  } else {
    pos -= 170;
    return ((uint32_t)(pos * 3) << 16) | ((uint32_t)(255 - pos * 3) << 8) | (0);
  }
}

// fast 8-bit random number generator shamelessly borrowed from FastLED
static uint8_t random8() {
  _rand16seed = (_rand16seed * 2053) + 13849;
  return (uint8_t)(((uint8_t)(_rand16seed & 0xFF)) +
                  ((uint8_t)(_rand16seed >> 8)));
}

static uint16_t random16(){
  _rand16seed = (_rand16seed * 2053) + 13849;
  return _rand16seed;
}

// static int32_t random32(int16_t delta) {
//   int32_t res = random8();
//   res = res << 8 | random8();
//   res = res << 8 | random8();
//   res = res << 8 | random8();
//   return res;
// }

static int32_t random32(uint16_t delta) {
  int32_t res = (int32_t)random16l(delta);
  return res;
}

static int32_t random32mm(int min, int max) {
  int32_t res = (int32_t)random16mm(min, max);
  return res;
}

// note random8(uint8_t) generates numbers in the range 0 - 254, 255 is never
// generated
uint8_t random8l(uint8_t lim) {
  uint8_t r = random8();
  r = (r * lim) >> 8;
  return r;
}

static uint16_t random16l( uint16_t lim)
{
    uint16_t r = random16();
    uint32_t p = (uint32_t)lim * (uint32_t)r;
    r = p >> 16;
    return r;
}

static uint8_t random8mm(uint8_t min, uint8_t lim)
{
    uint8_t delta = lim - min;
    uint8_t r = random8l(delta) + min;
    return r;
}

static uint16_t random16mm( uint16_t min, uint16_t lim)
{
    uint16_t delta = lim - min;
    uint16_t r = random16l( delta) + min;
    return r;
}


/*
 * Returns a new, random wheel index with a minimum distance of 42 from pos.
 */
static uint8_t get_random_wheel_index(uint8_t pos) {
  uint8_t r = 0;
  uint8_t x = 0;
  uint8_t y = 0;
  uint8_t d = 0;

  while (d < 42) {
    r = random8();
    x = abs(pos - r);
    y = 255 - x;
    d = min(x, y);
  }

  return r;
}

static inline void setPixelColor(int line, uint16_t i, uint32_t c) {  
  uint8_t r = (c >> 16) & 0xFF;
  uint8_t g = (c >> 8) & 0xFF;
  uint8_t b = c & 0xFF;
  led_set_color(line, i, r, g, b);
}

static inline void setPixelColorRGBW(int line, uint16_t i, uint8_t r, uint8_t g,
                                     uint8_t b, uint8_t w) {
  led_set_color(line, i, r, g, b);
}



/*
 * No blinking. Just plain old static light.
 */
uint16_t WS2812FX_mode_static(int line) {
  // uint32_t pixel_color = (fx_segments[line]->palette!=0) ? colorFromPalette(curPal, palette_size, numLeds, i) : fx_segments[line]->colors[0];

  // for (uint16_t i = fx_segments[line]->start; i <= fx_segments[line]->stop; i++) {
  //   // setPixelColor(line, i, fx_segments[line]->colors[0]);
  //   setPixelColor(line, i, ((fx_segments[line]->palette!=0) ? colorFromPalette(curPal, palette_size, numLeds, i) : fx_segments[line]->colors[0]));
  // }

  CREATEDEFAULTPALETTE(SEGCOLOR(0));

  for (uint16_t i = SEGMENT->start; i <= SEGMENT->stop; i++) {
    // setPixelColor(line, i, fx_segments[line]->colors[0]);
    setPixelColor(line, i, COLORFROMPALETTE(i-(SEGMENT->start)));
  }

  return 500;
}

#define REVERSE (uint8_t)0x80

static inline int isReverse(int line) {
  return (fx_segments[line]->options & REVERSE) == REVERSE;
}

#if 0
static inline int fadeRate(int line)
{
	return (fx_segments[line]->options & 0x70) >> 4;
}
#endif

/*
 * Blink/strobe function
 * Alternate between color1 and color2
 * if(strobe == true) then create a strobe effect
 */
static uint16_t blink(int line, uint32_t color1, uint32_t color2, int strobe) {
  CREATEDEFAULTPALETTE(color1);
  uint32_t color;
    
  for (uint16_t i = SEGMENT->start; i <= SEGMENT->stop; i++) {
    color = ((SEGMENT->counter_mode_call & 1) == 0) ? COLORFROMPALETTE(i-(SEGMENT->start)) : color2;
    if (isReverse(line))
      color = (color == color1) ? color2 : color1;
    setPixelColor(line, i, color);
  }


  // for (uint16_t i = fx_segments[line]->start; i <= fx_segments[line]->stop; i++) {
  //   if(SEGMENT->palette == 0)setPixelColor(line, i, color);
  //   if(SEGMENT->palette != 0){
  //     color1 = colorFromPalette(gGradientPalettes[SEGMENT->palette].palette, gGradientPalettes[SEGMENT->palette].size, SEGMENT->stop - SEGMENT->start, i);
  //     color =  ((fx_segments[line]->counter_mode_call & 1) == 0) ? color1 : color2;
  //     setPixelColor(line, i, color);
  //   }
  // }

  uint16_t delay = (200000/(fx_segments[line]->speed))+200;

  if ((fx_segments[line]->counter_mode_call & 1) == 0) {
    return strobe ? 20 : (delay / 2);
  } else {
    return strobe ? delay - 200
                  : (delay / 2);
  }
}

/*
 * Normal blinking. 50% on/off time.
 */
uint16_t WS2812FX_mode_blink(int line) {
  return blink(line, fx_segments[line]->colors[0], fx_segments[line]->colors[1],
               false);
}

/*
 * Classic Blink effect. Cycling through the rainbow.
 */
uint16_t WS2812FX_mode_blink_rainbow(int line) {
  return blink(line, color_wheel(fx_segments[line]->counter_mode_call & 0xFF),
               fx_segments[line]->colors[1], false);
}

/*
 * Classic Strobe effect.
 */
uint16_t WS2812FX_mode_strobe(int line) {
  return blink(line, fx_segments[line]->colors[0], fx_segments[line]->colors[1],
               true);
}

/*
 * Classic Strobe effect. Cycling through the rainbow.
 */
uint16_t WS2812FX_mode_strobe_rainbow(int line) {
  return blink(line, color_wheel(SEGMENT->counter_mode_call & 0xFF),
               fx_segments[line]->colors[1], true);
}

/*
 * Color wipe function
 * LEDs are turned on (color1) in sequence, then turned off (color2) in
 * sequence. if (int rev == true) then LEDs are turned off in reverse order
 */
static uint16_t color_wipe(int line, uint32_t color1, uint32_t color2,
                           int rev) {
  CREATEDEFAULTPALETTE(SEGCOLOR(color1));

  if (fx_segments[line]->counter_mode_step < segmentLength(line)) {
    uint32_t led_offset = fx_segments[line]->counter_mode_step;
    if (isReverse(line)) {  
      // setPixelColor(line, fx_segments[line]->stop - led_offset, color1);
      setPixelColor(line, fx_segments[line]->stop - led_offset, COLORFROMPALETTE((fx_segments[line]->stop - led_offset)-(SEGMENT->start))); //FIXME: Relative Pixel indexing
    } else {
      setPixelColor(line, fx_segments[line]->start + led_offset, COLORFROMPALETTE((fx_segments[line]->start + led_offset)-(SEGMENT->start)));
    }
  } else {
    uint32_t led_offset =
        fx_segments[line]->counter_mode_step - segmentLength(line);
    if ((isReverse(line) && !rev) || (!isReverse(line) && rev)) {
      setPixelColor(line, fx_segments[line]->stop - led_offset, color2);
    } else {
      setPixelColor(line, fx_segments[line]->start +  led_offset, color2);
    }
  }

  if (fx_segments[line]->counter_mode_step % segmentLength(line) == 0)
    setCycle(line);
  else
    clearCycle(line);

  fx_segments[line]->counter_mode_step =
      ((fx_segments[line]->counter_mode_step + 1) ) % (segmentLength(line) * 2);
  return 15*(2000/fx_segments[line]->speed);
}

/*
 * Color wipe function from WLED
 * LEDs are turned on (color1) in sequence, then turned off (color2) in sequence.
 * if (bool rev == true) then LEDs are turned off in reverse order
 */
static uint16_t color_wipe_wled(int line, bool rev, bool useRandomColors) {
  CREATEDEFAULTPALETTE(SEGCOLOR(0));
  uint32_t cycleTime = 50 + (255 - (int)(255*((float)SEGSPEED/(float)2000)));   //128 is segment speed FIXME:
  // uint32_t perc = strip.now % cycleTime;
  uint32_t perc = SEGMENT->next_time % cycleTime;
  uint16_t prog = (perc * 65535) / cycleTime;
  bool back = (prog > 32767);
  if (back) {
    prog -= 32767;
    if (SEGENV->counter_mode_step == 0) SEGENV->counter_mode_step = 1;
  } else {
    if (SEGENV->counter_mode_step == 2) SEGENV->counter_mode_step = 3; //trigger color change
  }

  if (useRandomColors) {
    if (SEGENV->counter_mode_call == 0) {
      SEGENV->aux_param = random8();
      SEGENV->counter_mode_step = 3;
    }
    if (SEGENV->counter_mode_step == 1) { //if flag set, change to new random color
      SEGENV->aux_param2 = get_random_wheel_index(SEGENV->aux_param);
      SEGENV->counter_mode_step = 2;
    }
    if (SEGENV->counter_mode_step == 3) {
      SEGENV->aux_param = get_random_wheel_index(SEGENV->aux_param2);
      SEGENV->counter_mode_step = 0;
    }
  }

  uint16_t ledIndex = (prog * SEGLEN) >> 15;
  uint16_t rem = 0;
  rem = (prog * SEGLEN) * 2; //mod 0xFFFF
  rem /= (SEGMENT->intensity +1);
  if (rem > 255) rem = 255;

  uint32_t col1 = useRandomColors? color_wheel(SEGENV->aux_param2) : SEGCOLOR(1);
  for (int i = 0; i < SEGLEN; i++)
  {
    uint16_t index = (rev && back)? SEGLEN -1 -i : i;
    uint32_t col0 = useRandomColors? color_wheel(SEGENV->aux_param) : COLORFROMPALETTE(index);

    if (i < ledIndex)
    {
      setPixelColor(line, index + SEGMENT->start, back? col1 : col0);
    } else
    {
      setPixelColor(line, index + SEGMENT->start, back? col0 : col1);
      if (i == ledIndex) setPixelColor(line, index + SEGMENT->start, color_blend((back? col0 : col1), (back? col1 : col0), rem));
    }
  }
  return 20;
  // return FRAMETIME;
}

/*
 * Lights all LEDs one after another.
 */
uint16_t WS2812FX_mode_color_wipe(int line) {
  return color_wipe(line, fx_segments[line]->colors[0],fx_segments[line]->colors[1], false);
}

uint16_t WS2812FX_mode_color_wipe_inv(int line) {
  return color_wipe(line, fx_segments[line]->colors[1],
                    fx_segments[line]->colors[0], false);
}

uint16_t WS2812FX_mode_color_wipe_rev(int line) {
  return color_wipe(line, fx_segments[line]->colors[0],
                    fx_segments[line]->colors[1], true);
}

uint16_t WS2812FX_mode_color_wipe_rev_inv(int line) {
  return color_wipe(line, fx_segments[line]->colors[1],
                    fx_segments[line]->colors[0], true);
}

/*
 * Turns all LEDs after each other to a random color.
 * Then starts over with another color.
 */
uint16_t WS2812FX_mode_color_wipe_random(int line) {
  // if (SEGENV->counter_mode_step % segmentLength(line) == 0) { // aux_param will store our random color wheel index
  //   SEGENV->aux_param = get_random_wheel_index(SEGENV->aux_param);
  // }
  // printf("\nAUX PARAM %d", SEGMENT->aux_param);
  // uint32_t color = color_wheel(SEGENV->aux_param);
  // // uint32_t color = WARMWHITE;
  // return color_wipe(line, color, color, false) * 2;

  return color_wipe_wled(line, false, true);

}

/*
 * Random color introduced alternating from start and end of strip.
 */
uint16_t WS2812FX_mode_color_sweep_random(int line) {
  // if (fx_segments[line]->counter_mode_step % segmentLength(line) ==
  //     0) { // aux_param will store our random color wheel index
  //   fx_segments[line]->aux_param =
  //       get_random_wheel_index(fx_segments[line]->aux_param);
  // }
  // uint32_t color = color_wheel(fx_segments[line]->aux_param);
  // return color_wipe(line, color, color, true) * 2;
  return color_wipe_wled(line, true, true) * 2;
}

/*
 * Lights all LEDs in one random color up. Then switches them
 * to the next random color.
 */
uint16_t WS2812FX_mode_random_color(int line) {
  fx_segments[line]->aux_param = get_random_wheel_index(
      fx_segments[line]->aux_param); // aux_param will store our random color wheel index
  uint32_t color = color_wheel(fx_segments[line]->aux_param);

  for (uint16_t i = fx_segments[line]->start; i <= fx_segments[line]->stop; i++) {
    setPixelColor(line, i, color);
  }
  return 50*(2000/fx_segments[line]->speed);
}

/*
 * Lights every LED in a random color. Changes one random LED after the other
 * to another random color.
 */
uint16_t WS2812FX_mode_single_dynamic(int line) {
  if (fx_segments[line]->counter_mode_call == 0) {
    for (uint16_t i = fx_segments[line]->start; i <= fx_segments[line]->stop;
         i++) {
      setPixelColor(line, i, color_wheel(random8()));
    }
  }

  setPixelColor(line, fx_segments[line]->start + random16l(segmentLength(line)),
                color_wheel(random8()));
  return 50*(2000/fx_segments[line]->speed);
}

/*
 * Lights every LED in a random color. Changes all LED at the same time
 * to new random colors.
 */
uint16_t WS2812FX_mode_multi_dynamic(int line) {
  for (uint16_t i = fx_segments[line]->start; i <= fx_segments[line]->stop; i++) {
    setPixelColor(line, i, color_wheel(random8()));
  }
  return 50*(2000/fx_segments[line]->speed);
}

/*
 * Does the "standby-breathing" of well known i-Devices. Fixed Speed.
 * Use mode "fade" if you like to have something similar with a different speed.
 */
uint16_t WS2812FX_mode_breath(int line) {
  CREATEDEFAULTPALETTE(SEGCOLOR(0));
  int lum = fx_segments[line]->counter_mode_step;
  uint8_t brightness =255;
  if (lum > 255)
    lum = 511 - lum; // lum = 15 -> 255 -> 15

  uint16_t delay;
  if (lum == 15){
    delay = 400; // 970 pause before each breath
    // brightness = 0;
  }
  else if (lum <= 25){
    delay = 38; // 19
    // brightness = 40;
  }
  else if (lum <= 50){
    delay = 36; // 18
    // brightness = 60;
  }
  else if (lum <= 75){
    delay = 28; // 14
    // brightness = 90; 
  }
  else if (lum <= 100){
    delay = 20; // 10
    // brightness = 130;
  }
  else if (lum <= 125){
    delay = 14; // 7
    // brightness = 170;
  }
  else if (lum <= 150){
    delay = 11; // 5
    // brightness = 190;
  }
  else{
    delay = 10; // 4
    // brightness = 255;
  }

  uint32_t color;
  uint8_t w,r,g,b;
  for (uint16_t i = SEGMENT->start; i <= SEGMENT->stop; i++) {
    color = COLORFROMPALETTE(i-(SEGMENT->start));

    // w = (color >> 24 & 0xFF) * lum / 256;
    // r = (color >> 16 & 0xFF) * lum / 256;
    // g = (color >> 8 & 0xFF) * lum / 256;
    // b = (color & 0xFF) * lum / 256;

    // setPixelColor(line, i , color);

    setPixelColorRGBW(line, i, (color >> 16 & 0xFF) * lum / 256, (color >> 8 & 0xFF) * lum / 256, (color & 0xFF) * lum / 256, (color >> 24 & 0xFF) * lum / 256);
    //  setPixelColor(line, i, (colorFromPalette(gGradientPalettes[SEGMENT->palette].palette, gGradientPalettes[SEGMENT->palette].size, SEGLEN, i-(SEGMENT->start)))*(lum/256));
  }

  fx_segments[line]->counter_mode_step += (uint32_t)(12*((SEGSPEED)/2000.));  //this 12 defines the speed 
  if (fx_segments[line]->counter_mode_step > (512 - 15))
    fx_segments[line]->counter_mode_step = 15;

  // delay *= 0.1;

  return delay;
}

/*
 * color blend function
 */
static uint32_t color_blend(uint32_t color1, uint32_t color2, uint8_t blend) {
  if (blend == 0)
    return color1;
  if (blend == 255)
    return color2;

  int w1 = (color1 >> 24) & 0xff;
  int r1 = (color1 >> 16) & 0xff;
  int g1 = (color1 >> 8) & 0xff;
  int b1 = color1 & 0xff;

  int w2 = (color2 >> 24) & 0xff;
  int r2 = (color2 >> 16) & 0xff;
  int g2 = (color2 >> 8) & 0xff;
  int b2 = color2 & 0xff;

  uint32_t w3 = ((w2 * blend) + (w1 * (255 - blend))) / 256;
  uint32_t r3 = ((r2 * blend) + (r1 * (255 - blend))) / 256;
  uint32_t g3 = ((g2 * blend) + (g1 * (255 - blend))) / 256;
  uint32_t b3 = ((b2 * blend) + (b1 * (255 - blend))) / 256;

  return ((w3 << 24) | (r3 << 16) | (g3 << 8) | (b3));
}

/*
 * Fades the LEDs between two colors
 */
uint16_t WS2812FX_mode_fade(int line) {

  //fill default palette with primary color
  if(SEGMENT->palette == 0){
    CREATEDEFAULTPALETTE(SEGCOLOR(0));
  }

  int lum = fx_segments[line]->counter_mode_step;
  if (lum > 255)
    lum = 511 - lum; // lum = 0 -> 255 -> 0

  for (uint16_t i = SEGMENT->start; i <= SEGMENT->stop; i++) {
    uint32_t color = color_blend(COLORFROMPALETTE(i-(SEGMENT->start)), SEGCOLOR(1), lum);
    setPixelColor(line, i, color);
  }

  fx_segments[line]->counter_mode_step += 8;
  if (fx_segments[line]->counter_mode_step > 511)
    fx_segments[line]->counter_mode_step = 0;
  return (fx_segments[line]->speed / 128);
}

/*
 * Runs a single pixel back and forth.
 */
uint16_t WS2812FX_mode_scan(int line) {

  CREATEDEFAULTPALETTE(SEGCOLOR(0));


  if (fx_segments[line]->counter_mode_step > (segmentLength(line) * 2) - 3) {
    fx_segments[line]->counter_mode_step = 0;
  }

  for (uint16_t i = fx_segments[line]->start; i <= fx_segments[line]->stop; i++) {
    setPixelColor(line, i, fx_segments[line]->colors[1]);
  }

  int led_offset =
      fx_segments[line]->counter_mode_step - (segmentLength(line) - 1);
  led_offset = abs(led_offset);

  if (isReverse(line)) {
    // setPixelColor(line, fx_segments[line]->stop - led_offset, fx_segments[line]->colors[0]);
    setPixelColor(line, fx_segments[line]->stop - led_offset, COLORFROMPALETTE(led_offset));
  } else {
    // setPixelColor(line, fx_segments[line]->start + led_offset, fx_segments[line]->colors[0]);
    setPixelColor(line, fx_segments[line]->start + led_offset, COLORFROMPALETTE(led_offset));
  }

  fx_segments[line]->counter_mode_step++;
  return (fx_segments[line]->speed / (segmentLength(line) * 2));
}

/*
 * Runs two pixel back and forth in opposite directions.
 */
uint16_t WS2812FX_mode_dual_scan(int line) {
  if (fx_segments[line]->counter_mode_step > (segmentLength(line) * 2) - 3) {
    fx_segments[line]->counter_mode_step = 0;
  }

  for (uint16_t i = fx_segments[line]->start; i <= fx_segments[line]->stop; i++) {
    setPixelColor(line, i, fx_segments[line]->colors[1]);
  }

  int led_offset =
      fx_segments[line]->counter_mode_step - (segmentLength(line) - 1);
  led_offset = abs(led_offset);

  setPixelColor(line, fx_segments[line]->start + led_offset,
                fx_segments[line]->colors[0]);
  setPixelColor(line,
                fx_segments[line]->start + segmentLength(line) - led_offset - 1,
                fx_segments[line]->colors[0]);

  fx_segments[line]->counter_mode_step++;
  return (fx_segments[line]->speed / (segmentLength(line) * 2));
}

/*
 * Cycles all LEDs at once through a rainbow.
 */
uint16_t WS2812FX_mode_rainbow(int line) {
  uint32_t color = color_wheel(SEGMENT->counter_mode_step);
  for (uint16_t i = SEGMENT->start; i <= SEGMENT->stop; i++) {
    setPixelColor(line, i, color);
  }

  SEGMENT->counter_mode_step =
      (SEGMENT->counter_mode_step + (int)(0.01*SEGMENT->speed)) & 0xFF;
  return 25*(2000/SEGMENT->speed);
}

/*
 * Cycles a rainbow over the entire string of LEDs.
 */
uint16_t WS2812FX_mode_rainbow_cycle(int line) {
  for (uint16_t i = 0; i < segmentLength(line); i++) {
    uint32_t color = color_wheel(((i * 256 / segmentLength(line)) +
                                  fx_segments[line]->counter_mode_step) &
                                 0xFF);
    setPixelColor(line, fx_segments[line]->start + i, color);
  }

  fx_segments[line]->counter_mode_step =
      // (fx_segments[line]->counter_mode_step + 1) & 0xFF;
      (SEGMENT->counter_mode_step + (int)(0.01*SEGMENT->speed)) & 0xFF;
  return 25*(2000/SEGMENT->speed);
}

/*
 * theater chase function
 */
static uint16_t theater_chase(int line, uint32_t color1, uint32_t color2) {
  fx_segments[line]->counter_mode_call = fx_segments[line]->counter_mode_call % 3;
  for (uint16_t i = 0; i < segmentLength(line); i++) {
    if ((i % 3) == fx_segments[line]->counter_mode_call) {
      if (isReverse(line)) {
        setPixelColor(line, fx_segments[line]->stop - i, color1);
      } else {
        setPixelColor(line, fx_segments[line]->start + i, color1);
      }
    } else {
      if (isReverse(line)) {
        setPixelColor(line, fx_segments[line]->stop - i, color2);
      } else {
        setPixelColor(line, fx_segments[line]->start + i, color2);
      }
    }
  }

  return (uint16_t)(fx_segments[line]->speed / (float)segmentLength(line));
}

/*
 * Theatre-style crawling lights.
 * Inspired by the Adafruit examples.
 */
uint16_t WS2812FX_mode_theater_chase(int line) {
  return theater_chase(line, fx_segments[line]->colors[0],
                       fx_segments[line]->colors[1]);
}

/*
 * Theatre-style crawling lights with rainbow effect.
 * Inspired by the Adafruit examples.
 */
uint16_t WS2812FX_mode_theater_chase_rainbow(int line) {
  fx_segments[line]->counter_mode_step =
      (fx_segments[line]->counter_mode_step + 1) & 0xFF;
  return theater_chase(line, color_wheel(fx_segments[line]->counter_mode_step),
                       BLACK);
}

/*
 * Running lights effect with smooth sine transition.
 */
uint16_t WS2812FX_mode_running_lights(int line) {

  CREATEDEFAULTPALETTE(SEGCOLOR(0));

  uint32_t color;
  uint8_t r,g,b,w;

  // uint8_t w = ((fx_segments[line]->colors[0] >> 24) & 0xFF);
  // uint8_t r = ((fx_segments[line]->colors[0] >> 16) & 0xFF);
  // uint8_t g = ((fx_segments[line]->colors[0] >> 8) & 0xFF);
  // uint8_t b = (fx_segments[line]->colors[0] & 0xFF);

  uint8_t sineIncr = max(1, (256 / segmentLength(line)));

  for (uint16_t i = 0; i < segmentLength(line); i++) {
    int lum = (int)sine8(((i + fx_segments[line]->counter_mode_step) * sineIncr));
    color = COLORFROMPALETTE(i);

    if (isReverse(line)) {
      // setPixelColorRGBW(line, fx_segments[line]->start + i, (r * lum) / 256, (g * lum) / 256, (b * lum) / 256, (w * lum) / 256);
      setPixelColorRGBW(line, SEGMENT->start + i, (color >> 16 & 0xFF) * lum / 256, (color >> 8 & 0xFF) * lum / 256, (color & 0xFF) * lum / 256, (color >> 24 & 0xFF) * lum / 256);

    } else {
      // setPixelColorRGBW(line, fx_segments[line]->stop - i, (r * lum) / 256,  (g * lum) / 256, (b * lum) / 256, (w * lum) / 256);
      setPixelColorRGBW(line, SEGMENT->stop - i, (color >> 16 & 0xFF) * lum / 256, (color >> 8 & 0xFF) * lum / 256, (color & 0xFF) * lum / 256, (color >> 24 & 0xFF) * lum / 256);
    }
  }

  fx_segments[line]->counter_mode_step =
      (fx_segments[line]->counter_mode_step + 1) % 256;
  // return (fx_segments[line]->speed / segmentLength(line));
  return 15*(2000/fx_segments[line]->speed);
}

/*
 * twinkle function
 */
static uint16_t twinkle(int line, uint32_t color1, uint32_t color2) {
  if (fx_segments[line]->counter_mode_step == 0) {
    for (uint16_t i = fx_segments[line]->start; i <= fx_segments[line]->stop;
         i++) {
      setPixelColor(line, i, color2);
    }
    uint16_t min_leds =
        max(1, segmentLength(line) / 5); // make sure, at least one LED is on
    uint16_t max_leds =
        max(1, segmentLength(line) / 2); // make sure, at least one LED is on
    fx_segments[line]->counter_mode_step = random32mm(min_leds, max_leds);
  }

  setPixelColor(line, fx_segments[line]->start + random16l(segmentLength(line)),
                color1);

  fx_segments[line]->counter_mode_step--;
  return (fx_segments[line]->speed / segmentLength(line));
}

/*
 * Blink several LEDs on, reset, repeat.
 * Inspired by www.tweaking4all.com/hardware/arduino/arduino-led-strip-effects/
 */
uint16_t WS2812FX_mode_twinkle(int line) {
  return twinkle(line, fx_segments[line]->colors[0],
                 fx_segments[line]->colors[1]);
}

/*
 * Blink several LEDs in random colors on, reset, repeat.
 * Inspired by www.tweaking4all.com/hardware/arduino/arduino-led-strip-effects/
 */
uint16_t WS2812FX_mode_twinkle_random(int line) {
  return twinkle(line, color_wheel(random8()), fx_segments[line]->colors[1]);
}

/*
 * fade out function
 */
#if 0
static void fade_out(int line) {
	static const uint8_t rateMapH[] = {0, 1, 1, 1, 2, 3, 4, 6};
	static const uint8_t rateMapL[] = {0, 2, 3, 8, 8, 8, 8, 8};

	uint8_t rate = fadeRate(line);
	uint8_t rateH = rateMapH[rate];
	uint8_t rateL = rateMapL[rate];

	uint32_t color = fx_segments[line]->colors[1]; // target color
	int w2 = (color >> 24) & 0xff;
	int r2 = (color >> 16) & 0xff;
	int g2 = (color >> 8) & 0xff;
	int b2 = color & 0xff;

	for (uint16_t i = fx_segments[line]->start; i <= fx_segments[line]->stop; i++) {
		color = getPixelColor(line,i);
		if (rate == 0) { // old fade-to-black algorithm
			setPixelColor(line,i, (color >> 1) & 0x7F7F7F7F);
		} else { // new fade-to-color algorithm
			int w1 = (color >> 24) & 0xff;// current color
			int r1 = (color >> 16) & 0xff;
			int g1 = (color >> 8) & 0xff;
			int b1 = color & 0xff;

			// calculate the color differences between the current and target colors
			int wdelta = w2 - w1;
			int rdelta = r2 - r1;
			int gdelta = g2 - g1;
			int bdelta = b2 - b1;

			// if the current and target colors are almost the same, jump right to the target color,
			// otherwise calculate an intermediate color. (fixes rounding issues)
			wdelta =
			abs(wdelta) < 3 ?
			wdelta : (wdelta >> rateH) + (wdelta >> rateL);
			rdelta =
			abs(rdelta) < 3 ?
			rdelta : (rdelta >> rateH) + (rdelta >> rateL);
			gdelta =
			abs(gdelta) < 3 ?
			gdelta : (gdelta >> rateH) + (gdelta >> rateL);
			bdelta =
			abs(bdelta) < 3 ?
			bdelta : (bdelta >> rateH) + (bdelta >> rateL);

			setPixelColorRGBW(line,i, r1 + rdelta, g1 + gdelta, b1 + bdelta,
					w1 + wdelta);
		}
	}
}

/*
 * twinkle_fade function
 */
static uint16_t twinkle_fade(int line, uint32_t color) {
	fade_out(line);

	if (random8(3) == 0) {
		setPixelColor(line,fx_segments[line]->start + random16l(segmentLength(line)), color);
	}
	return (fx_segments[line]->speed / 8);
}

/*
 * Blink several LEDs on, fading out.
 */
uint16_t WS2812FX_mode_twinkle_fade(int line) {
	return twinkle_fade(line,fx_segments[line]->colors[0]);
}

/*
 * Blink several LEDs in random colors on, fading out.
 */
uint16_t WS2812FX_mode_twinkle_fade_random(int line) {
	return twinkle_fade(line,color_wheel(random8()));
}
#endif

/*
 * Blinks one LED at a time.
 * Inspired by www.tweaking4all.com/hardware/arduino/arduino-led-strip-effects/
 */
uint16_t WS2812FX_mode_sparkle(int line) {
  setPixelColor(line, fx_segments[line]->start + fx_segments[line]->aux_param3,
                fx_segments[line]->colors[1]);
  fx_segments[line]->aux_param3 =
      random16l(segmentLength(line)); // aux_param3 stores the random led index
  setPixelColor(line, fx_segments[line]->start + fx_segments[line]->aux_param3,
                fx_segments[line]->colors[0]);
  // return (fx_segments[line]->speed / segmentLength(line));
  return 25*(2000/SEGMENT->speed);
}

/*
 * Lights all LEDs in the color. Flashes single white pixels randomly.
 * Inspired by www.tweaking4all.com/hardware/arduino/arduino-led-strip-effects/
 */
uint16_t WS2812FX_mode_flash_sparkle(int line) {
  CREATEDEFAULTPALETTE(SEGCOLOR(0));
  if (1) {
    for (uint16_t i = fx_segments[line]->start; i <= fx_segments[line]->stop;
         i++) {
      setPixelColor(line, i, COLORFROMPALETTE(i-(SEGMENT->start)));
    }
  }

  setPixelColor(line, fx_segments[line]->start + fx_segments[line]->aux_param3,
                fx_segments[line]->colors[0]);

  if (random8l(5) == 0) {
    fx_segments[line]->aux_param3 =
        random16l(segmentLength(line)); // aux_param3 stores the random led index//FIXME: random32 is returning pixels greater than the segment length
    setPixelColor(line, fx_segments[line]->start + fx_segments[line]->aux_param3,
                  WHITE);
    return 20;
  }
  // return fx_segments[line]->speed;
  return 15*(2000/fx_segments[line]->speed);
}

/*
 * Like flash sparkle. With more flash.
 * Inspired by www.tweaking4all.com/hardware/arduino/arduino-led-strip-effects/
 */
uint16_t WS2812FX_mode_hyper_sparkle(int line) {
  CREATEDEFAULTPALETTE(SEGCOLOR(0));
  for (uint16_t i = fx_segments[line]->start; i <= fx_segments[line]->stop; i++) {
    setPixelColor(line, i, COLORFROMPALETTE(i-(SEGMENT->start)));
  }

  if (random8l(5) < 2) {
    for (uint16_t i = 0; i < max(1, segmentLength(line) / 3); i++) {
      setPixelColor(line, fx_segments[line]->start + random16l(segmentLength(line)),
                    WHITE);
    }
    return 20;
  }
  return 15*(2000/fx_segments[line]->speed);
}

/*
 * Strobe effect with different strobe count and pause, controlled by speed.
 */
uint16_t WS2812FX_mode_multi_strobe(int line) {
  CREATEDEFAULTPALETTE(SEGCOLOR(0));
  for (uint16_t i = fx_segments[line]->start; i <= fx_segments[line]->stop; i++) {
    setPixelColor(line, i, COLORFROMPALETTE(i-(SEGMENT->start)));
  }

  uint16_t delay = 200 + ((9 - (fx_segments[line]->speed % 10)) * 100);
  uint16_t count = 2 * ((fx_segments[line]->speed / 100) + 1);
  if (fx_segments[line]->counter_mode_step < count) {
    if ((fx_segments[line]->counter_mode_step & 1) == 0) {
      for (uint16_t i = fx_segments[line]->start; i <= fx_segments[line]->stop;
           i++) {
        setPixelColor(line, i, fx_segments[line]->colors[1]);
      }
      delay = 20;
    } else {
      delay = 50;
    }
  }
  fx_segments[line]->counter_mode_step =
      (fx_segments[line]->counter_mode_step + 1) % (count + 1);
  return delay;
}


//   fx_segments[line]->counter_mode_step =
//       (fx_segments[line]->counter_mode_step + 2) % (count + 1);   
//   return delay;
// }

static uint16_t police_strobe(int line, uint32_t color1, uint32_t color2) {
  if (SEGLEN == 1) return WS2812FX_mode_static(line);
  uint16_t delay = 1 + (20<<3) / SEGLEN;  // longer segments should change faster
  // uint16_t delay = 1 + (FRAMETIME<<3) / SEGLEN;  // longer segments should change faster
  uint32_t it = SEGENV->next_time;
  // uint32_t it = SEGENV->next_time / map(SEGSPEED, 0, 2000 , delay<<4, delay);
  uint16_t offset = it % SEGLEN;

  uint16_t width = ((SEGLEN*(SEGMENT->intensity+1))>>9); //max width is half the strip
  if (!width) width = 1;
  for (int i = 0; i < width; i++) {
    uint16_t indexR = (offset + i) % SEGLEN;
    uint16_t indexB = (offset + i + (SEGLEN>>1)) % SEGLEN;
    setPixelColor(line, indexR + SEGMENT->start, color1);
    setPixelColor(line, indexB + SEGMENT->start, color2);
  }
  // return 10;
  return 265 - map(SEGSPEED, 0, 2000 , 0, 255);
  // return FRAMETIME;
}

/*
 * Police Strobe effect with different strobe count and pause, controlled by speed.
 */
uint16_t WS2812FX_mode_police_strobe(int line) {
  // CREATEDEFAULTPALETTE(SEGCOLOR(0));

  // //turn all off
  // for (uint16_t i = fx_segments[line]->start; i <= fx_segments[line]->stop; i++) {
  //   setPixelColor(line, i, 0);
  // }

  // uint16_t delay = 200 + ((9 - (fx_segments[line]->speed % 10)) * 100); //209
  // uint16_t count = 2 * ((fx_segments[line]->speed / 100) + 1);  //2*11 = 22

  // if ((fx_segments[line]->counter_mode_step > 0) && (fx_segments[line]->counter_mode_step < count/2)) {
  //   if ((fx_segments[line]->counter_mode_step & 1) == 0) {
  //     for (uint16_t i = fx_segments[line]->start; i <= fx_segments[line]->stop; i++) {
  //       if(i <= (SEGLEN)/2)
  //         setPixelColor(line, i, COLORFROMPALETTE(i-(SEGMENT->start)));
  //     }
  //     delay = 20;
  //   } else {
  //     delay = 50;
  //   }
  // }  

  // if ((fx_segments[line]->counter_mode_step > count/2) && (fx_segments[line]->counter_mode_step < count)) {
  //   if ((fx_segments[line]->counter_mode_step & 1) == 0) {
  //     for (uint16_t i = fx_segments[line]->start; i <= fx_segments[line]->stop; i++) {
  //       if(i >= (SEGLEN)/2)
  //         setPixelColor(line, i, fx_segments[line]->colors[1]);
  //     }
  //     delay = 20;
  //   } else {
  //     delay = 50;
  //   }
  return police_strobe(line, RED, BLUE);
  }  

/*
 * Android loading circle
 */
uint16_t WS2812FX_mode_android(int line) {

  CREATEDEFAULTPALETTE(SEGCOLOR(0));

  for (int i = 0; i < SEGLEN; i++) { //Bg Color
    int j = i+SEGMENT->start;
    setPixelColor(line, j, SEGCOLOR(1));
  }

  if (SEGENV->aux_param2 > (SEGMENT->intensity*SEGLEN)/255)
  {
    SEGENV->aux_param = 1;
  } else
  {
    if (SEGENV->aux_param2 < 2) SEGENV->aux_param = 0;
  }

  uint16_t a = SEGENV->counter_mode_step;

  if (SEGENV->aux_param == 0)
  {
    if (SEGENV->counter_mode_call %3 == 1) {a++;}
    else {SEGENV->aux_param2++;}
  } else
  {
    a++;
    if (SEGENV->counter_mode_call %3 != 1) SEGENV->aux_param2--;
  }

  if (a >= SEGLEN) a = 0;

  if (a + SEGENV->aux_param2 < SEGLEN)
  {
    for (int i = a; i < a+SEGENV->aux_param2; i++) {
      int j = i+SEGMENT->start;
      // setPixelColor(line, j, SEGCOLOR(0));
      setPixelColor(line, j, COLORFROMPALETTE(i));
    }
  } else
  {
    for (int i = a; i < SEGLEN; i++) {
      int j = i+SEGMENT->start;
      // setPixelColor(line, j, SEGCOLOR(0));
      setPixelColor(line, j, COLORFROMPALETTE(i));
    }
    for (int i = 0; i < SEGENV->aux_param2 - (SEGLEN -a); i++) {
      int j = i+SEGMENT->start;
      // setPixelColor(line, j, SEGCOLOR(0));
      setPixelColor(line, j, COLORFROMPALETTE(i));
    }

  }
  SEGENV->counter_mode_step = a;

  // return 3 + ((16 * (uint32_t)(2000 -SEGMENT->speed)) / SEGLEN);
  return 15*(2000/SEGSPEED);
  // return 20;
}

/*
 * color chase function.
 * color1 = background color
 * color2 and color3 = colors of two adjacent leds
 */

static uint16_t chase(int line, uint32_t color1, uint32_t color2,
                      uint32_t color3) {

  CREATEDEFAULTPALETTE(color1);

  uint16_t a = fx_segments[line]->counter_mode_step;
  uint16_t b = (a + 1) % segmentLength(line);
  uint16_t c = (b + 1) % segmentLength(line);

  color1 = COLORFROMPALETTE(a);

  if (isReverse(line)) {
    setPixelColor(line, fx_segments[line]->stop - a, color1);
    setPixelColor(line, fx_segments[line]->stop - b, color2);
    setPixelColor(line, fx_segments[line]->stop - c, color3);
  } else {
    setPixelColor(line, fx_segments[line]->start + a, color1);
    setPixelColor(line, fx_segments[line]->start + b, color2);
    setPixelColor(line, fx_segments[line]->start + c, color3);
  }

  if (b == 0)
    setCycle(line);
  else
    clearCycle(line);

  fx_segments[line]->counter_mode_step =
      (fx_segments[line]->counter_mode_step + 1) % segmentLength(line);
  return (10 + (255 - map(SEGSPEED, 0, 2000, 0, 255))*0.5);
}

/*
 * Bicolor chase mode
 */
uint16_t WS2812FX_mode_bicolor_chase(int line) {
  return chase(line, fx_segments[line]->colors[0], fx_segments[line]->colors[1],
               fx_segments[line]->colors[2]);
}

/*
 * Fire flicker function
 */
static uint16_t fire_flicker(int line, int rev_intensity) {
  CREATEDEFAULTPALETTE(SEGCOLOR(0));
  uint32_t color;
  uint8_t r,g,b,w;
  // uint8_t w = (fx_segments[line]->colors[0] >> 24) & 0xFF;
  // uint8_t r = (fx_segments[line]->colors[0] >> 16) & 0xFF;
  // uint8_t g = (fx_segments[line]->colors[0] >> 8) & 0xFF;
  // uint8_t b = (fx_segments[line]->colors[0] & 0xFF);

  for (uint16_t i = SEGMENT->start; i <= SEGMENT->stop; i++) {
    color = COLORFROMPALETTE(i-(SEGMENT->start));

    w = (color >> 24) & 0xFF;
    r = (color >> 16) & 0xFF;
    g = (color >> 8) & 0xFF;
    b = (color & 0xFF);

    uint8_t lum = max(w, max(r, max(g, b))) / rev_intensity;
    int flicker = random8(lum);
    setPixelColorRGBW(line, i, max(r - flicker, 0), max(g - flicker, 0),
                      max(b - flicker, 0), max(w - flicker, 0));
  }
  // return (SEGSPEED / segmentLength(line));
  return (10 + (255 - map(SEGSPEED, 0, 2000, 0, 255))*0.5);
}

/*
 * White running on _color.
 */
uint16_t WS2812FX_mode_chase_color(int line) {
  return chase(line, fx_segments[line]->colors[0], WHITE, WHITE);
}

/*
 * Black running on _color.
 */
uint16_t WS2812FX_mode_chase_blackout(int line) {
  return chase(line, fx_segments[line]->colors[0], BLACK, BLACK);
}

/*
 * _color running on white.
 */
uint16_t WS2812FX_mode_chase_white(int line) {
  return chase(line, WHITE, fx_segments[line]->colors[0],
               fx_segments[line]->colors[0]);
}

/*
 * White running followed by random color.
 */
uint16_t WS2812FX_mode_chase_random(int line) {
  if (fx_segments[line]->counter_mode_step == 0) {
    fx_segments[line]->aux_param =
        get_random_wheel_index(fx_segments[line]->aux_param);
  }
  return chase(line, color_wheel(fx_segments[line]->aux_param), WHITE, WHITE);
}

/*
 * Rainbow running on white.
 */
uint16_t WS2812FX_mode_chase_rainbow_white(int line) {
  uint16_t n = fx_segments[line]->counter_mode_step;
  uint16_t m = (fx_segments[line]->counter_mode_step + 1) % segmentLength(line);
  uint32_t color2 = color_wheel(((n * 256 / segmentLength(line)) +
                                 (fx_segments[line]->counter_mode_call & 0xFF)) &
                                0xFF);
  uint32_t color3 = color_wheel(((m * 256 / segmentLength(line)) +
                                 (fx_segments[line]->counter_mode_call & 0xFF)) &
                                0xFF);

  return chase(line, WHITE, color2, color3);
}

/*
 * White running on rainbow.
 */
uint16_t WS2812FX_mode_chase_rainbow(int line) {
  uint8_t color_sep = 256 / segmentLength(line);
  uint8_t color_index = fx_segments[line]->counter_mode_call & 0xFF;
  uint32_t color = color_wheel(
      ((fx_segments[line]->counter_mode_step * color_sep) + color_index) & 0xFF);

  return chase(line, color, WHITE, WHITE);
}

/*
 * Black running on rainbow.
 */
uint16_t WS2812FX_mode_chase_blackout_rainbow(int line) {
  uint8_t color_sep = 256 / segmentLength(line);
  uint8_t color_index = fx_segments[line]->counter_mode_call & 0xFF;
  uint32_t color = color_wheel(
      ((fx_segments[line]->counter_mode_step * color_sep) + color_index) & 0xFF);

  return chase(line, color, BLACK, BLACK);
}

/*
 * White flashes running on _color.
 */
uint16_t WS2812FX_mode_chase_flash(int line) {
  CREATEDEFAULTPALETTE(SEGCOLOR(0));

  const static uint8_t flash_count = 4;
  uint8_t flash_step =
      fx_segments[line]->counter_mode_call % ((flash_count * 2) + 1);

  for (uint16_t i = fx_segments[line]->start; i <= fx_segments[line]->stop; i++) {
    setPixelColor(line, i, COLORFROMPALETTE(i-(SEGMENT->start)));
    // setPixelColor(line, i, WHITE);
  }

  uint16_t delay = (fx_segments[line]->speed / segmentLength(line));
  if (flash_step < (flash_count * 2)) {
    if (flash_step % 2 == 0) {
      uint16_t n = fx_segments[line]->counter_mode_step;
      uint16_t m =
          (fx_segments[line]->counter_mode_step + 1) % segmentLength(line);
      if (isReverse(line)) {
        setPixelColor(line, fx_segments[line]->stop - n, SEGCOLOR(1));
        setPixelColor(line, fx_segments[line]->stop - m, SEGCOLOR(1));

        // setPixelColor(line, SEGMENT->stop - n, COLORFROMPALETTE(n));
        // setPixelColor(line, SEGMENT->stop - m, COLORFROMPALETTE(m));
        
      } else {
        setPixelColor(line, fx_segments[line]->start + n, SEGCOLOR(1));
        setPixelColor(line, fx_segments[line]->start + m, SEGCOLOR(1));

        // setPixelColor(line, SEGMENT->start + n, COLORFROMPALETTE(n));
        // setPixelColor(line, SEGMENT->start + m, COLORFROMPALETTE(m));
      }
      delay = 20;
    } else {
      delay = 30;
    }
  } else {
    fx_segments[line]->counter_mode_step =
        (fx_segments[line]->counter_mode_step + 1) % segmentLength(line);
  }
  // return delay;
    return (5 + (255 - map(SEGSPEED, 0, 2000, 0, 255))*0.5);
}

/*
 * White flashes running, followed by random color.
 */
uint16_t WS2812FX_mode_chase_flash_random(int line) {

  const static uint8_t flash_count = 4;
  uint8_t flash_step =
      fx_segments[line]->counter_mode_call % ((flash_count * 2) + 1);

  for (uint16_t i = 0; i < fx_segments[line]->counter_mode_step; i++) {
    setPixelColor(line, fx_segments[line]->start + i,
                  color_wheel(fx_segments[line]->aux_param));
  }

  uint16_t delay = (fx_segments[line]->speed / segmentLength(line));
  if (flash_step < (flash_count * 2)) {
    uint16_t n = fx_segments[line]->counter_mode_step;
    uint16_t m =
        (fx_segments[line]->counter_mode_step + 1) % segmentLength(line);
    if (flash_step % 2 == 0) {
      setPixelColor(line, fx_segments[line]->start + n, WHITE);
      setPixelColor(line, fx_segments[line]->start + m, WHITE);
      delay = 20;
    } else {
      setPixelColor(line, fx_segments[line]->start + n,
                    color_wheel(fx_segments[line]->aux_param));
      setPixelColor(line, fx_segments[line]->start + m, BLACK);
      delay = 30;
    }
  } else {
    fx_segments[line]->counter_mode_step =
        (fx_segments[line]->counter_mode_step + 1) % segmentLength(line);

    if (fx_segments[line]->counter_mode_step == 0) {
      fx_segments[line]->aux_param =
          get_random_wheel_index(fx_segments[line]->aux_param);
    }
  }
  // return delay;
    return (5 + (255 - map(SEGSPEED, 0, 2000, 0, 255))*0.2);
}

/*
 * Alternating pixels running function.
 */
static uint16_t running(int line, uint32_t color1, uint32_t color2) {
  for (uint16_t i = 0; i < segmentLength(line); i++) {
    if ((i + fx_segments[line]->counter_mode_step) % 4 < 2) {
      if (isReverse(line)) {
        setPixelColor(line, fx_segments[line]->start + i, color1);
      } else {
        setPixelColor(line, fx_segments[line]->stop - i, color1);
      }
    } else {
      if (isReverse(line)) {
        setPixelColor(line, fx_segments[line]->start + i, color2);
      } else {
        setPixelColor(line, fx_segments[line]->stop - i, color2);
      }
    }
  }

  fx_segments[line]->counter_mode_step =
      (fx_segments[line]->counter_mode_step + 1) & 0x3;
  // return (fx_segments[line]->speed / segmentLength(line));
  return (10 + (255 - map(SEGSPEED, 0, 2000, 0, 255))*0.5);
}

/*
 * Alternating color/white pixels running.
 */
uint16_t WS2812FX_mode_running_color(int line) {
  return running(line, fx_segments[line]->colors[0], WHITE);
}

/*
 * Alternating red/blue pixels running.
 */
uint16_t WS2812FX_mode_running_red_blue(int line) {
  return running(line, RED, BLUE);
}

/*
 * Alternating red/green pixels running.
 */
uint16_t WS2812FX_mode_merry_christmas(int line) {
  return running(line, RED, GREEN);
}

/*
 * Alternating orange/purple pixels running.
 */
uint16_t WS2812FX_mode_halloween(int line) {
  return running(line, PURPLE, ORANGE);
}

#if 0
/*
 * Random colored pixels running.
 */
uint16_t WS2812FX_mode_running_random(int line) {
	for (uint16_t i = segmentLength(line) - 1; i > 0; i--) {
		if (isReverse(line)) {
			setPixelColor(line,fx_segments[line]->stop - i,
					getPixelColor(line,fx_segments[line]->stop - i + 1));
		} else {
			setPixelColor(line,fx_segments[line]->start + i,
					getPixelColor(line,fx_segments[line]->start + i - 1));
		}
	}

	if (fx_segments[line]->counter_mode_step == 0) {
		fx_segments[line]->aux_param = get_random_wheel_index(
				fx_segments[line]->aux_param);
		if (isReverse(line)) {
			setPixelColor(line,fx_segments[line]->stop, color_wheel(fx_segments[line]->aux_param));
		} else {
			setPixelColor(line,fx_segments[line]->start,
					color_wheel(fx_segments[line]->aux_param));
		}
	}

	fx_segments[line]->counter_mode_step =
	(fx_segments[line]->counter_mode_step == 0) ? 1 : 0;
	return (fx_segments[line]->speed / segmentLength(line));
}

/*
 * K.I.T.T.
 */
uint16_t WS2812FX_mode_larson_scanner(int line) {
	fade_out(line);

	if (fx_segments[line]->counter_mode_step < segmentLength(line)) {
		if (isReverse(line)) {
			setPixelColor(line,fx_segments[line]->stop - fx_segments[line]->counter_mode_step,
					fx_segments[line]->colors[0]);
		} else {
			setPixelColor(line,fx_segments[line]->start + fx_segments[line]->counter_mode_step,
					fx_segments[line]->colors[0]);
		}
	} else {
		if (isReverse(line)) {
			setPixelColor(line,
					fx_segments[line]->stop
					- ((segmentLength(line) * 2)
							- fx_segments[line]->counter_mode_step) + 2,
					fx_segments[line]->colors[0]);
		} else {
			setPixelColor(line,
					fx_segments[line]->start
					+ ((segmentLength(line) * 2)
							- fx_segments[line]->counter_mode_step) - 2,
					fx_segments[line]->colors[0]);
		}
	}

	if (fx_segments[line]->counter_mode_step % segmentLength(line) == 0)
	setCycle(line);
	else
	clearCycle(line);

	fx_segments[line]->counter_mode_step = (fx_segments[line]->counter_mode_step + 1)
	% ((segmentLength(line) * 2) - 2);
	return (fx_segments[line]->speed / (segmentLength(line) * 2));
}

/*
 * Firing comets from one end.
 */
uint16_t WS2812FX_mode_comet(int line) {
	fade_out(line);

	if (isReverse(line)) {
		setPixelColor(line,fx_segments[line]->stop - fx_segments[line]->counter_mode_step,
				fx_segments[line]->colors[0]);
	} else {
		setPixelColor(line,fx_segments[line]->start + fx_segments[line]->counter_mode_step,
				fx_segments[line]->colors[0]);
	}

	fx_segments[line]->counter_mode_step = (fx_segments[line]->counter_mode_step + 1)
	% segmentLength(line);
	return (fx_segments[line]->speed / segmentLength(line));
}

/*
 * Fireworks function.
 */
static uint16_t fireworks(int line, uint32_t color) {
	fade_out(line);

//// set brightness(i) = brightness(i-1)/4 + brightness(i) + brightness(i+1)/4
	/*
	 // the old way, so many calls to the pokey getPixelColor() function made this super slow
	 for(uint16_t i=fx_segments[line]->start + 1; i <fx_segments[line]->stop; i++) {
	 uint32_t prevLed = (Adafruit_NeoPixel_getPixelColor(i-1) >> 2) & 0x3F3F3F3F;
	 uint32_t thisLed = Adafruit_NeoPixel_getPixelColor(i);
	 uint32_t nextLed = (Adafruit_NeoPixel_getPixelColor(i+1) >> 2) & 0x3F3F3F3F;
	 setPixelColor(line,i, prevLed + thisLed + nextLed);
	 }
	 */

// the new way, manipulate the Adafruit_NeoPixels pixels[] array directly, about 5x faster
	uint8_t *pixels = getPixels();
	uint8_t pixelsPerLed = (wOffset == rOffset) ? 3 : 4;// RGB or RGBW device
	uint16_t startPixel = fx_segments[line]->start * pixelsPerLed + pixelsPerLed;
	uint16_t stopPixel = fx_segments[line]->stop * pixelsPerLed;
	for (uint16_t i = startPixel; i < stopPixel; i++) {
		uint16_t tmpPixel = (pixels[i - pixelsPerLed] >> 2) + pixels[i]
		+ (pixels[i + pixelsPerLed] >> 2);
		pixels[i] = tmpPixel > 255 ? 255 : tmpPixel;
	}

	if (!_triggered) {
		for (uint16_t i = 0; i < max(1, segmentLength(line) / 20); i++) {
			if (random8(10) == 0) {
				setPixelColor(line,fx_segments[line]->start + random32(segmentLength(line)), color);
			}
		}
	} else {
		for (uint16_t i = 0; i < max(1, segmentLength(line) / 10); i++) {
			setPixelColor(line,fx_segments[line]->start + random32(segmentLength(line)), color);
		}
	}
	return (fx_segments[line]->speed / segmentLength(line));
}

/*
 * Firework sparks.
 */
uint16_t WS2812FX_mode_fireworks(int line) {
	return fireworks(fx_segments[line]->colors[0]);
}

/*
 * Random colored firework sparks.
 */
uint16_t WS2812FX_mode_fireworks_random(void) {
	return fireworks(color_wheel(random8()));
}
#endif

/*
 * Random flickering.
 */
uint16_t WS2812FX_mode_fire_flicker(int line) { return fire_flicker(line, 3); }

/*
 * Random flickering, less intensity.
 */
uint16_t WS2812FX_mode_fire_flicker_soft(int line) {
  return fire_flicker(line, 6);
}

/*
 * Random flickering, more intensity.
 */
uint16_t WS2812FX_mode_fire_flicker_intense(int line) {
  return fire_flicker(line, 1.7);
}

/*
 * Tricolor chase function
 */
static uint16_t tricolor_chase(int line, uint32_t color1, uint32_t color2,
                               uint32_t color3) {
  CREATEDEFAULTPALETTE(color1);
  uint16_t index = fx_segments[line]->counter_mode_step % 6;
  for (uint16_t i = 0; i < segmentLength(line); i++, index++) {
    if (index > 5)
      index = 0;

    uint32_t color = color3;
    if (index < 2){
      color1 = COLORFROMPALETTE(i);
      color = color1;
    }
    else if (index < 4){
      color = color2;
    }

    if (isReverse(line)) {
      setPixelColor(line, fx_segments[line]->start + i, color);
    } else {
      setPixelColor(line, fx_segments[line]->stop - i, color);
    }
  }

  fx_segments[line]->counter_mode_step++;
  // return (fx_segments[line]->speed / segmentLength(line));
  return (10 + (255 - map(SEGSPEED, 0, 2000, 0, 255))*0.5);
}

/*
 * Tricolor chase mode
 */
uint16_t WS2812FX_mode_tricolor_chase(int line) {
  return tricolor_chase(line, fx_segments[line]->colors[0],
                        fx_segments[line]->colors[1],
                        fx_segments[line]->colors[2]);
}

/*
 * Alternating white/red/black pixels running.
 */
uint16_t WS2812FX_mode_circus_combustus(int line) {
  return tricolor_chase(line, SEGCOLOR(0), SEGCOLOR(1), SEGCOLOR(2));
}

/*
 * ICU mode
 */
uint16_t WS2812FX_mode_icu(int line) {
  CREATEDEFAULTPALETTE(SEGCOLOR(0));
  int virtualSegmentLength = 5;
  if(SEGMENT->counter_mode_step + virtualSegmentLength  >= SEGLEN){
    SEGMENT->counter_mode_step = 0;
  }
  uint16_t dest = fx_segments[line]->counter_mode_step ;


  // setPixelColor(line, fx_segments[line]->start + dest,
  //               fx_segments[line]->colors[0]);
  // setPixelColor(line, fx_segments[line]->start + dest + segmentLength(line) / 2,
  //               fx_segments[line]->colors[0]);

  setPixelColor(line, fx_segments[line]->start + dest,
                COLORFROMPALETTE(dest));
  setPixelColor(line, fx_segments[line]->start + dest + virtualSegmentLength,
                COLORFROMPALETTE(dest));

  if (fx_segments[line]->aux_param3 == dest) { // pause between eye movements
    if (random8(6) == 0) {                    // blink once in a while
      setPixelColor(line, fx_segments[line]->start + dest, BLACK);
      setPixelColor(line,
                    fx_segments[line]->start + dest + virtualSegmentLength,
                    BLACK);
      return 200;
    }
    // fx_segments[line]->aux_param3 = random16l(segmentLength(line) / 2);
    fx_segments[line]->aux_param3 = random16l(SEGLEN/2);
    return 1000 + random16l(2000);
  }

  setPixelColor(line, fx_segments[line]->start + dest, BLACK);
  setPixelColor(line, fx_segments[line]->start + dest + virtualSegmentLength,
                BLACK);

  if (fx_segments[line]->aux_param3 > fx_segments[line]->counter_mode_step) {
    fx_segments[line]->counter_mode_step++;
    dest++;
  } else if (fx_segments[line]->aux_param3 <
             fx_segments[line]->counter_mode_step) {
    fx_segments[line]->counter_mode_step--;
    dest--;
  }

  setPixelColor(line, fx_segments[line]->start + dest,
                COLORFROMPALETTE(dest));
  setPixelColor(line, fx_segments[line]->start + dest + virtualSegmentLength,
                COLORFROMPALETTE(dest));

  // return (fx_segments[line]->speed / segmentLength(line));
  return 15*(2000/fx_segments[line]->speed);
}
