#include "FAB_LED.h"
#include <avr/power.h>

#define N 4
#define MAX 128
#define AC 100

#define ADC_PORT 3
#define ADC_PIN 3

/*
  Hardware
*/

ws2812<B,4>  leds;
grbw  pixels[N] = {};

int lmin = 20; // 1024;
int lmax = 100; // 1;

inline uint8_t val()
{
  int val = analogRead(ADC_PORT);

  // lmin = (val < lmin) ? val : lmin;
  // lmax = (val > lmax) ? val : lmax;
    
  val = ((lmax - lmin) > 0) ? ((val - lmin) * MAX / (lmax - lmin)) : 0;  
  val = (val > MAX) ? MAX : ( (val < 0) ? 0 : val);
  
  return (uint8_t) val;
}

void updateColors(char r, char g, char b)
{
  for(int i = 0; i < N; i++)
  {
    pixels[i].r = r;
    pixels[i].g = g;
    pixels[i].b = b;
  }
}

inline void set()
{
  leds.sendPixels(N, pixels);
}

/*
  Utils
*/

void hsv(int i, uint16_t h, uint8_t s, uint8_t v)
{
    uint8_t f = (h % 60) * 255 / 60;
    uint8_t p = (255 - s) * (uint16_t)v / 255;
    uint8_t q = (255 - f * (uint16_t)s / 255) * (uint16_t)v / 255;
    uint8_t t = (255 - (255 - f) * (uint16_t)s / 255) * (uint16_t)v / 255;
    uint8_t r = 0, g = 0, b = 0;
    switch((h / 60) % 6){
        case 0: r = v; g = t; b = p; break;
        case 1: r = q; g = v; b = p; break;
        case 2: r = p; g = v; b = t; break;
        case 3: r = p; g = q; b = v; break;
        case 4: r = t; g = p; b = v; break;
        case 5: r = v; g = p; b = q; break;
    }

    pixels[i].r = r;
    pixels[i].g = g;
    pixels[i].b = b;
}

inline uint8_t add_limit(uint8_t s, int d) {
  int v = ((int) s) + d;
  return (v > MAX) ? MAX : (v < 0) ? 0 : ((uint8_t) v); 
}

void fade(int n, int steps, int duration, int r, int g, int b)
{
  for(int i; i < steps; i++) {
    pixels[n].r = add_limit(pixels[n].r, r);
    pixels[n].g = add_limit(pixels[n].g, g);
    pixels[n].b = add_limit(pixels[n].b, b);
    set();  
    delay(duration);
  }
}

/*
  Effects
*/

void startup()
{
  leds.clear(N);
  fade(0, 10, 30, 0, MAX / 9, MAX / 9);
  fade(0, 10, 30, 0, -MAX / 9, -MAX / 9);
  fade(1, 10, 30, 0, MAX / 9, 0);
  fade(1, 10, 30, 0, -MAX / 9, 0);
  fade(2, 10, 30, 0, 0, MAX / 9);
  fade(2, 10, 30, 0, 0, -MAX / 9);
  fade(3, 10, 30, MAX / 9, 0, 0);
  fade(3, 10, 30, -MAX / 9, 0, 0);
  leds.clear(N);
}

void rainbow(uint8_t v)
{
  uint16_t time = millis() >> 2;
  
  for(int i = 0; i < N; i++) {
    byte x = (time >> 2) - (i << 3);
    hsv(i, (uint32_t)x * 359 / 256, 255, v);
  }
  set();
}

void weight_rainbow(uint8_t v)
{
  static int k = 0;
  static long int m = 0;
  static int weight[N] = {1, 1, 1};
  uint16_t time = millis() >> 2;
  int i;
  
  if(k % AC == AC - 1) {
    for(i = N - 1; i > 0; i--) weight[i] = weight[i - 1];
    weight[i] = (int) ((m + (long int) v) / (long int)AC);
    k = 0;
    m = 0;
  } else {
    m = m + v;
    k++;
  }
    
  for(i = 0; i < N; i++) {
    byte x = (time >> 2) - (i << 3);
    hsv(i, (uint32_t)x * 359 / 256, 255, weight[i]);
  }
  set();
}

void color_loop(uint8_t v) 
{
  static char ts[3 * N] = {0,0,0,0,0,0,0,0,0};
  int i;

  for(i = 3 * N - 1; i > 0; i--) ts[i] = ts[i - 1];
  ts[0] = (char) v;
  
  for(i = 0; i < N; i++)
  {
    pixels[i].r = ts[i * 3];
    pixels[i].g = ts[i * 3 + 1];
    pixels[i].b = ts[i * 3 + 2];
  }
  set();
  delay(100);
}

void color_loop_hsv(uint8_t v) 
{
  static char ts[N] = {0,0,0};
  int i;

  for(i = N - 1; i > 0; i--) ts[i] = ts[i - 1];
  ts[0] = (char) v;
  
  for(i = 0; i < N; i++)
  {
    hsv(i, (uint32_t)ts[i] * 359 / 256, 255, 255);
  }
  set();
}

void color_jump_hsv(uint8_t v) 
{
  static int k = 0;
  uint8_t r,g,b;

  hsv(k, (uint32_t)v * 359 / 256, 255, 255);
  r = pixels[k].r / 8;  
  g = pixels[k].g / 8;
  b = pixels[k].b / 8; 

  pixels[k].r = 0;
  pixels[k].g = 0;
  pixels[k].b = 0;

  fade(k, 16, 10, r, g, b);
  fade(k, 8, 10, -r*2, -g*2, -b*2);

  k = (k + 1) % N;
}


/*
  Arduino
*/

void setup()
{
  clock_prescale_set(clock_div_1);

  pinMode(ADC_PIN, INPUT);
  leds.clear(2 * N);
  startup();
}


void loop()
{
  static int count = 0;
  static int mode = 0; 
  
  int v = val();
  if(v < 50) {
    count++;
  } else {
    count = 0;
  }
  if(count > 60) {
    mode = (mode + 1) % 7;
    count = 0;
  }
       
  switch(mode) {
    case(1):
     color_loop(v);
     break;
    case(2):
     weight_rainbow(MAX - v);
     break;
    case(3):
     color_loop_hsv(v);
     break;
    case(4):
     color_jump_hsv(v);
     break;
    default:
     rainbow(MAX - v);            
  }
}

