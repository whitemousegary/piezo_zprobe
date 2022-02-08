
////////////////////////////////////////////////////////////
// free variables, customizable 
//
  float target_freq     = 2.0;   // kHz, [0.3, 2.5]
  float pin_high_peroid = 0.04;  // second
  float threshold_coef  = 3.0;   // irrelevant unless CALIBRATE_ME_ONLOAD is defined, 
                                 // threshold = background signal * threshold_coef
//
////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////
// hardware dependent const, from calibration
//
int v1mean = 2753;             // ADC reading average, 
                               //   [0,1024) if 10 bit ADC
                               //   [0,4096) if 12 bit ADC
float freq = 32.14;            // kHz, respond frequency
unsigned int threshold  = 3480; // [0,65536), signal signal 2-norm threshold, 
                               // twice the background signal is recommended
int wsize = constrain( int(freq/target_freq/2+.5)*4, 8, 64);
//
////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////
// Perform calibration, or not
// CAUTION!!! If CALIBRATE_ME_ONLOAD is defined, 
// v1mean, freq, threshold and wsize will be overrode.
//
#define CALIBRATE_ME_ONLOAD
//
////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////
// operation variables
//
  int COUNT = freq*pin_high_peroid*1000;
  int count = 0;      // [0,COUNT)
  int count1 = 0;      // [0,COUNT)
  int nacc0 = 0;      // [0,wsize)
  int scof[64];       // [-256,256], sampled sin function
  int ccof[64];       // [-256,256], sampled cos function
  int sbuf[64];       // [-2048,2048], v1*sin/256
  int cbuf[64];       // [-2048,2048], v1*cos/256
  int sacc0 = 0;      // [-2048*wsize,2048*wsize], sbuf sum
  int cacc0 = 0;      // [-2048*wsize,2048*wsize], cbuf sum
  unsigned int ssig;  // [0,65536), signal 2-norm
//
////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////
// for reporting or calibration
//
  bool v1mean_init = false;
  bool finit = false;
  bool winit = false;
  bool tinit = false;
  bool report = false;
    long numSamples = 0;
    long t0;
    long v1acc = 0;
    unsigned int max_signal = 0; // [0,65536), background signal 2-norm; just for 
                                   // reporting/calibration, not being used during operation
//
////////////////////////////////////////////////////////////

void cal_fcof( int w );
void init_adc();
void disable_adc_interrupt();
void enable_adc_interrupt();
void prepare_report();


void setup()
{
  pinMode(13, OUTPUT);
  digitalWrite(13,LOW);
  Serial.begin(115200);
  init_adc();

  #ifdef CALIBRATE_ME_ONLOAD
    disable_adc_interrupt();
      Serial.print("Calibrating variables...\n");
      wsize = 12;
    enable_adc_interrupt();
    delay(100);
    prepare_report();
  #else
    disable_adc_interrupt();
      Serial.print("Initialing variables... ");
      v1mean_init = true;
      winit = true;
      tinit = true;
      finit = true;
      cal_fcof( wsize );
      Serial.print("done\n");
    enable_adc_interrupt();
  #endif
}
void loop()
{
  if( Serial.available() )
  {
    int ch = Serial.read();
    if( ch=='p' )
    {
      disable_adc_interrupt();
        Serial.print("Preparing report...\n");
      enable_adc_interrupt();
      delay(100);
      prepare_report();
    }else if( ch=='a' )
    {
      disable_adc_interrupt();
        wsize = int(freq/2/2+.5)*4;
        cal_fcof( wsize );
        Serial.print("wsize ");
        Serial.print(wsize);
        Serial.println();
      enable_adc_interrupt();
    }else if( ch=='1' || ch=='2' || ch=='3' )
    {
      disable_adc_interrupt();
        if( ch=='1' )
          threshold = float(max_signal);
        if( ch=='2' )
          threshold = float(threshold)*.9;
        if( ch=='3' )
          threshold = float(threshold)/.9;
        Serial.print("threshold [");
        Serial.print(threshold);
        Serial.print("/");
        Serial.print(65535);
        Serial.println("]");
      enable_adc_interrupt();
    }
  }
  if(report)
  {
    if(micros()-t0>2000000)
    {
      report = false;
      if( !v1mean_init )
      {
        disable_adc_interrupt();
          v1mean = v1acc/numSamples;
          Serial.print(  "  ADC reading average: ");
          Serial.println(v1mean);
          v1mean_init = true;
        enable_adc_interrupt();
        delay(100);
        prepare_report();
      }else if( !finit )
      {
        disable_adc_interrupt();
          freq = float(numSamples)/(micros()-t0)*1000;
          COUNT = constrain( freq*pin_high_peroid*1000, 1, INT16_MAX );
          Serial.print(  "  Respond frequency: ");
          Serial.print(freq);
          Serial.println(" kHz");
          finit = true;
        enable_adc_interrupt();
        delay(100);
        prepare_report();
      }else if(!winit)
      {
        disable_adc_interrupt();
          wsize = constrain( int(freq/target_freq/2+.5)*4, 8, 64);
          cal_fcof( wsize );
          Serial.print( "  Window size ");
          Serial.print(wsize);
          Serial.print(" <= for target_freq ");
          Serial.print(target_freq);
          Serial.println(" kHz");
          winit = true;
        enable_adc_interrupt();
        delay(100);
        prepare_report();
      }else if(!tinit)
      {
        disable_adc_interrupt();
          threshold = float(max_signal) *threshold_coef;
          Serial.print( "  Threshold [" );
          Serial.print(threshold);
          Serial.print("/");
          Serial.print(65535);
          Serial.print("]");
          Serial.print(" <= background signal ");
          Serial.print(max_signal);
          Serial.print("/");
          Serial.print(65535);
          Serial.print("]");
          Serial.println();
          Serial.println("done");
          tinit = true;
        enable_adc_interrupt();
      }else
      {
        disable_adc_interrupt();
          Serial.print(  "  ADC reading average: ");
          Serial.println(v1mean);
          Serial.print(  "  Respond frequency: ");
          Serial.print(freq);
          Serial.println(" kHz");
          Serial.print( "  Window size ");
          Serial.print(wsize);
          Serial.print(" <= for target_freq ");
          Serial.print(target_freq);
          Serial.println(" kHz");
          Serial.print("  max_signal [");
          Serial.print(max_signal);
          Serial.print("/");
          Serial.print(65535);
          Serial.println("]");
          Serial.println("done");
        enable_adc_interrupt();
      }
    }
    delay(100);
  }
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
  sval = constrain(v1,0,wsize) * scof[nacc0]/256;
  cval = constrain(v1,0,wsize) * ccof[nacc0]/256;
  sbuf[nacc0] = sval;
  cbuf[nacc0] = cval;
  sacc0 += sval;
  cacc0 += cval;
  nacc0 = (nacc0+1)%wsize;

  sval = min(abs(sacc0),255);
  cval = min(abs(cacc0),255);
  ssig = sval*sval + cval*cval;

  bool trigger = false;
  if( ssig>threshold )
    trigger = true;

  if( trigger && count==0 && count1==0 )
  {
    count = COUNT;
    count1 = 2*wsize;
    if( !report )
    PORTB |= (1<<PB5);
  }
  
  count = count>0 ? count-1 : 0;
  if( count==0 )
  {
    PORTB &= ~(1<<PB5);
    count1 = count1>0 ? count1-1 : 0;
  }
  
  if(report)
  {
    numSamples++;
    if(max_signal<ssig)
      max_signal = max_signal<ssig ? ssig : max_signal;
  }
  //Serial.println( sacc0 );
}


void cal_fcof( int w )
{
  int i;
  for( i=0; i<w; i++ )
  {
    float x = float(i+.5)/(w) * 4*PI;
    scof[i] = sin(x)*256;
    ccof[i] = cos(x)*256;
    sbuf[i] = 0;
    cbuf[i] = 0;
  }
  sacc0 = 0;
  cacc0 = 0;
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
void disable_adc_interrupt()
{
  ADCSRA &= ~(1 << ADATE); // disable auto trigger
  ADCSRA &= ~(1 << ADIE);  // disable interrupts when measurement complete
  ADCSRA &= ~(1 << ADEN);  // disable ADC
  ADCSRA &= ~(1 << ADSC);  // stop ADC measurements
}
void enable_adc_interrupt()
{
  ADCSRA |= (1 << ADATE); // enable auto trigger
  ADCSRA |= (1 << ADIE);  // enable interrupts when measurement complete
  ADCSRA |= (1 << ADEN);  // enable ADC
  ADCSRA |= (1 << ADSC);  // start ADC measurements
}

void prepare_report()
{
  t0 = micros();
  numSamples = 0;
  v1acc = 0;
  max_signal = 0;
  report = true;
}
