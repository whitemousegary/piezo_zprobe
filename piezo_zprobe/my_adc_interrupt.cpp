#include <arduino.h>

#include "my_variables.h"
#include "my_eeprom.h"
#include "my_adc_interrupt.h"


////////////////////////////////////////////////////////////
// operation variables
//
  int wsize = 0;
  int COUNT = 0;
  int count_pin_high   = 0;      // [0,COUNT)
  int count_pin_slient = 0;      // [0,2*wsize)
  int nacc0 = 0;      // [0,wsize)
  int scof[64];       // [-127,127], sampled sin function
  int ccof[64];       // [-127,127], sampled cos function
  int sbuf[64];       // [-2048,2048], v1*sin/256
  int cbuf[64];       // [-2048,2048], v1*cos/256
  int sacc0 = 0;      // [-2048*wsize,2048*wsize], sbuf sum
  int cacc0 = 0;      // [-2048*wsize,2048*wsize], cbuf sum
  unsigned int ssig;  // [0,65536), signal 2-norm
//
////////////////////////////////////////////////////////////

void init_filter( float freq, float target_freq, float pin_high_peroid )
{
  COUNT = constrain( freq*pin_high_peroid*1000, 1, INT16_MAX );

  int peroid = min( int(freq/target_freq/2+.5)*2, 64 );
  int ncycle = 1;//64/peroid;

//        Serial.print("  peroid: ");
//          Serial.print(peroid);
//        Serial.print("\n");
//        Serial.print("  ncycle: ");
//          Serial.print(ncycle);
//        Serial.print("\n");
  
  wsize = peroid * ncycle;
  cal_fcof( wsize, ncycle );
}

void cal_fcof( int w, int ncycle )
{
  int i;
  for( i=0; i<w; i++ )
  {
    float x = float(i+.5)/(w) * 2*PI * ncycle;
    scof[i] = round(sin(x)*127);
    ccof[i] = round(cos(x)*127);
    sbuf[i] = 0;
    cbuf[i] = 0;
  }
  sacc0 = 0;
  cacc0 = 0;
}

void enable_adc_interrupt()
{
  ADCSRA |= (1 << ADATE); // enable auto trigger
  ADCSRA |= (1 << ADIE);  // enable interrupts when measurement complete
  ADCSRA |= (1 << ADEN);  // enable ADC
  ADCSRA |= (1 << ADSC);  // start ADC measurements
}
void disable_adc_interrupt()
{
  ADCSRA &= ~(1 << ADATE); // disable auto trigger
  ADCSRA &= ~(1 << ADIE);  // disable interrupts when measurement complete
  ADCSRA &= ~(1 << ADEN);  // disable ADC
  ADCSRA &= ~(1 << ADSC);  // stop ADC measurements
}

void init_adc()
{
  ADCSRA = 0;             // clear ADCSRA register
  ADCSRB = 0;             // clear ADCSRB register
  ADMUX |= (0 & 0x07);    // set A0 analog input pin
  ADMUX |= (1 << REFS0);  // set reference voltage
  //ADMUX |= (1 << ADLAR);  // left align ADC value to 8 bits from ADCH register
  // sampling rate is [ADC clock] / [prescaler] / [conversion clock cycles]
  // for Arduino Uno ADC clock is 16 MHz and a conversion takes 13 clock cycles
  //ADCSRA |= (1 << ADPS2) | (1 << ADPS0);    // 32 prescaler for 38.5 KHz
  ADCSRA |= (1 << ADPS2);                     // 16 prescaler for 76.9 KHz
  //ADCSRA |= (1 << ADPS1) | (1 << ADPS0);    // 8 prescaler for 153.8 KHz
  enable_adc_interrupt();
}

void prepare_calibration( bool v1mean_init1, bool finit1, bool tinit1 )
{
  if(v1mean_init1 && finit1 && tinit1 )
    return;
  disable_adc_interrupt();
    SERIAL_PRINT("Calibrating variables...\n");
    v1mean_init = v1mean_init1;
    finit = finit1;
    tinit = tinit1;
  enable_adc_interrupt();
  delay(100);
  prepare_report();
}

void prepare_report()
{
  t0 = micros();
  numSamples = 0;
  v1acc = 0;
  max_signal = 0;
  report = true;
}

ISR(ADC_vect)
{
  int v1 = int(ADCL)+int(ADCH)*256;
  if(!v1mean_init)
    v1acc += v1;
  v1 -= v1mean;

  int sval, cval;
  sacc0 -= sbuf[nacc0];
  cacc0 -= cbuf[nacc0];
  sval = (constrain(v1,-wsize,wsize) * scof[nacc0])/256;
  cval = (constrain(v1,-wsize,wsize) * ccof[nacc0])/256;
  sbuf[nacc0] = sval;
  cbuf[nacc0] = cval;
  sacc0 += sval;
  cacc0 += cval;
  nacc0 = (nacc0+1)%wsize;

  sval = min(abs(sacc0),127);
  cval = min(abs(cacc0),127);
  ssig = sval*sval + cval*cval;

  bool trigger = false;
  if( ssig>threshold )
    trigger = true;

  if( trigger && count_pin_high==0 && count_pin_slient==0 )
  {
    count_pin_high = COUNT;
    count_pin_slient= 2*wsize;
    if( !report )
    PORTB |= (1<<PB5);
  }
  
  count_pin_high = count_pin_high>0 ? count_pin_high-1 : 0;
  if( count_pin_high==0 )
  {
    if( !report )
      PORTB &= ~(1<<PB5);
    count_pin_slient= count_pin_slient>0 ? count_pin_slient-1 : 0;
  }
  
  if(report)
  {
    PORTB |= (1<<PB5);
    numSamples++;
    if(max_signal<ssig)
      max_signal = max_signal<ssig ? ssig : max_signal;
  }
  //Serial.println( sacc0 );
}
