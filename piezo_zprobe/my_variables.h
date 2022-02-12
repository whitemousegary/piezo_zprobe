#ifndef MY_VARIABLES_H
#define MY_VARIABLES_H 

//#define USE_SERIAL 
#define USE_EEPROM 
#define USE_BUTTONS 

#ifdef USE_SERIAL
  #define SERIAL_PRINT(a) Serial.print(a)
#else
  #define SERIAL_PRINT(a) 
#endif

extern int v1mean;
extern float freq;
extern unsigned int threshold;
extern bool v1mean_init;
extern bool finit;
extern bool tinit;

extern bool report;
extern long numSamples;
extern long t0;
extern long v1acc;
extern unsigned int max_signal;

extern int wsize;
extern int COUNT;

#endif
