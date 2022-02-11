#include "my_variables.h"
#include "my_eeprom.h"
#include "my_buttons.h"
#include "my_adc_interrupt.h"

////////////////////////////////////////////////////////////
// free variables, customizable 
//
  float target_freq     = 2.0;   // kHz, [0.3, 2.5]
  float pin_high_peroid = 0.04;  // second
  float threshold_coef  = 2.0;   // only used during calibration, 
                                 // threshold = background signal * threshold_coef
//
////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////
// hardware dependent const, from calibration
//
  int v1mean = 2757;              // ADC reading average, 
                                  //   [0,1024) if 10 bit ADC
                                  //   [0,4096) if 12 bit ADC
  float freq = 31.80;             // kHz, respond frequency
  unsigned int threshold  = 1758; // [0,65536), signal signal 2-norm threshold, 
                                  // twice the background signal is recommended
  bool v1mean_init = true;
  bool finit = true;
  bool tinit = true;
  //////////////////////////////////////////////////////////
  // CAUTIOUS!!! these values will be overrode unless 
  // USE_MY_VALUES is defined.
  // #define USE_MY_VALUES 
  //////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////
// for reporting or calibration
//
  bool report = false;
  long numSamples = 0;
  long t0;
  long v1acc = 0;
  unsigned int max_signal = 0; // [0,65536), background signal 2-norm; just for 
                                 // reporting/calibration, not being used during operation
//
////////////////////////////////////////////////////////////



void setup()
{

  
  Serial.begin(115200);
  pinMode(13, OUTPUT);
  digitalWrite(13,LOW);
  buttons_init();
  init_adc();

  disable_adc_interrupt();
    Serial.print("EEPROM: ");
      Serial.print(check_eeprom() ? "got something" : "empty");
    Serial.print("\n");
  enable_adc_interrupt();
  
#ifdef USE_MY_VALUES
  disable_adc_interrupt();
    Serial.print("Initialing variables... ");
      init_filter(freq, target_freq, pin_high_peroid);
    Serial.print("done\n\n");
  enable_adc_interrupt();
#else  
  if(check_eeprom())
  {
    disable_adc_interrupt();
      Serial.print("Initialing variables from EEPROM... ");
        load_eeprom();
        init_filter(freq, target_freq, pin_high_peroid);
      Serial.print("done\n\n");
    enable_adc_interrupt();
  }else
  {
    prepare_calibration();
  }
#endif
  
}
void loop()
{
  button_pressed();
  
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
    }
    else if( ch=='l' )
    {
      disable_adc_interrupt();
        Serial.print("Variable list:\n");
        Serial.print("  ADC reading average: ");
          Serial.print(v1mean);
        Serial.print("\n");
        Serial.print("  Respond frequency: ");
          Serial.print(freq);
        Serial.print("\n");
        Serial.print("  Threshold: [");
          Serial.print(threshold);
          Serial.print("/");
          Serial.print(65535);
        Serial.print("]\n");
        Serial.print("  wsize: ");
          Serial.print(wsize);
        Serial.print("\n");
        Serial.print("  COUNT: ");
          Serial.print(COUNT);
        Serial.print("\n");
        Serial.print("done\n\n");
      enable_adc_interrupt();
    }
    else if( ch=='1' || ch=='2' || ch=='3' )
    {
      disable_adc_interrupt();
        if( ch=='1' )
          threshold = float(max_signal);
        if( ch=='2' )
          threshold = float(threshold)*.9;
        if( ch=='3' )
          threshold = float(threshold)/.9;
        save_eeprom();
          Serial.print("  Threshold: [");
            Serial.print(threshold);
            Serial.print("/");
            Serial.print(65535);
          Serial.print("]\n");
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
          v1mean_init = true;
          Serial.print("  ADC reading average: ");
            Serial.print(v1mean);
          Serial.print("\n");
        enable_adc_interrupt();
        delay(100);
        prepare_report();
      }else if( !finit )
      {
        disable_adc_interrupt();
          freq = float(numSamples)/(micros()-t0)*1000;
          finit = true;
          init_filter(freq, target_freq, pin_high_peroid);
          Serial.print("  Respond frequency: ");
            Serial.print(freq);
          Serial.print(" kHz\n");
        enable_adc_interrupt();
        delay(100);
        prepare_report();
      }else if(!tinit)
      {
        disable_adc_interrupt();
          threshold = float(max_signal) *threshold_coef;
          tinit = true;
          Serial.print("  Threshold: [");
            Serial.print(threshold);
            Serial.print("/");
            Serial.print(65535);
            Serial.print("] <= background signal [");
            Serial.print(max_signal);
            Serial.print("/");
            Serial.print(65535);
            Serial.print("]\n");
          Serial.print("done\n\n");
          save_eeprom();
        enable_adc_interrupt();
      }else
      {
        disable_adc_interrupt();
          Serial.print("  Respond frequency: ");
            Serial.print(freq );
          Serial.print("kHz\n");
          Serial.print("  max_signal: [");
            Serial.print(max_signal);
            Serial.print("/");
            Serial.print(65535);
          Serial.print("] kHz\n");
          Serial.print("done\n\n");
        enable_adc_interrupt();
      }
    }
    delay(100);
  }
}
