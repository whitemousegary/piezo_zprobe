#include "my_variables.h"

////////////////////////////////////////////////////////////
// free variables, customizable 
//
  float target_freq     = 2.0;   // kHz, [0.3, 2.5], only used during calibration or reporting
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
  unsigned int threshold  = 2704; // [0,65536), signal signal 2-norm threshold, 
                                  // twice the background signal is recommended
  bool v1mean_init = 0;  // Set 1 to use the predefined v1mean, 
                         // 0 for calibration or eeprom initization
  bool finit =       0;  // 1 to use the predefined freq, 
                         // 0 for calibration or eeprom initization
  bool tinit =       0;  // Set 1 to use the predefined threshold,
                         // 0 for calibration or eeprom initization
//
////////////////////////////////////////////////////////////




#include "my_eeprom.h"
#include "my_buttons.h"
#include "my_adc_interrupt.h"

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
  #ifdef USE_SERIAL
    Serial.begin(115200);
  #endif
  #ifdef USE_EEPROM
    SERIAL_PRINT("EEPROM: ");
      SERIAL_PRINT(check_eeprom() ? "got something" : "empty");
    SERIAL_PRINT("\n");
  #endif
  #ifdef USE_BUTTONS
    buttons_init();
  #endif
  
  pinMode(13, OUTPUT);
  digitalWrite(13,LOW);
  init_adc();

  if(check_eeprom())
  {
    disable_adc_interrupt();
      SERIAL_PRINT("Initialing variables from EEPROM... ");
        load_eeprom();
        init_filter(freq, target_freq, pin_high_peroid);
      SERIAL_PRINT("done\n\n");
    enable_adc_interrupt();
  }else
  {
    disable_adc_interrupt();
      init_filter(freq, target_freq, pin_high_peroid);
    enable_adc_interrupt();
    prepare_calibration(v1mean_init,finit,tinit);
  }
}
void loop()
{
  #ifdef USE_BUTTONS
    button_pressed();
  #endif
  
  #ifdef USE_SERIAL
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
  #endif
  
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
          SERIAL_PRINT("  ADC reading average: ");
            SERIAL_PRINT(v1mean);
          SERIAL_PRINT("\n");
        enable_adc_interrupt();
        delay(100);
        prepare_report();
      }else if( !finit )
      {
        disable_adc_interrupt();
          freq = float(numSamples)/(micros()-t0)*1000;
          finit = true;
          init_filter(freq, target_freq, pin_high_peroid);
          SERIAL_PRINT("  Respond frequency: ");
            SERIAL_PRINT(freq);
          SERIAL_PRINT(" kHz\n");
        enable_adc_interrupt();
        delay(100);
        prepare_report();
      }else if(!tinit)
      {
        disable_adc_interrupt();
          threshold = float(max_signal) *threshold_coef;
          tinit = true;
          SERIAL_PRINT("  Threshold: [");
            SERIAL_PRINT(threshold);
            SERIAL_PRINT("/");
            SERIAL_PRINT(65535);
            SERIAL_PRINT("] <= background signal [");
            SERIAL_PRINT(max_signal);
            SERIAL_PRINT("/");
            SERIAL_PRINT(65535);
            SERIAL_PRINT("]\n");
          SERIAL_PRINT("done\n\n");
          save_eeprom();
        enable_adc_interrupt();
      }else
      {
        disable_adc_interrupt();
          SERIAL_PRINT("  Respond frequency: ");
            SERIAL_PRINT(freq );
          SERIAL_PRINT("kHz\n");
          SERIAL_PRINT("  max_signal: [");
            SERIAL_PRINT(max_signal);
            SERIAL_PRINT("/");
            SERIAL_PRINT(65535);
          SERIAL_PRINT("] kHz\n");
          SERIAL_PRINT("done\n\n");
        enable_adc_interrupt();
      }
    }
    delay(100);
  }
}
