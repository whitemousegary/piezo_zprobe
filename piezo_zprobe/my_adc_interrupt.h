#ifndef MY_ADC_INTERRUPT_H
#define MY_ADC_INTERRUPT_H

void prepare_calibration();
void prepare_report();

void init_adc();
void init_filter( float freq, float target_freq, float pin_high_peroid );

void cal_fcof( int w, int ncycle );
void disable_adc_interrupt();
void enable_adc_interrupt();

#endif
