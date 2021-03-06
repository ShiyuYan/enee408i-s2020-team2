#include "Ultrasound.h"
#include <Ewma.h>

void trigger_pulse();
void echo_interrupt_left();
void echo_interrupt_center();
void echo_interrupt_right();

volatile int trigger_time_count = 0;
volatile int trigger_state = 0;
volatile int pulse_schedule;
int _trigPin1;
int _echoPin1;
int _trigPin2;
int _echoPin2;
int _trigPin3;
int _echoPin3;
int _pulseWidth;
volatile long _echo_start_left;
volatile long _echo_end_left;
volatile long _echo_start_center;
volatile long _echo_end_center;
volatile long _echo_start_right;
volatile long _echo_end_right; 

volatile Ewma leftFilter(0.5);
volatile Ewma centerFilter(0.5);
volatile Ewma rightFilter(0.5);

// Class Constructor
Ultrasound::Ultrasound(int trigPin1, int echoPin1, int trigPin2, int echoPin2, int trigPin3, int echoPin3) {
    Ultrasound(trigPin1, echoPin1, trigPin2, echoPin2, trigPin3, echoPin3, DEFAULT_PULSEWIDTH, DEFAULT_PULSE_SCHEDULE);
}

Ultrasound::Ultrasound(int trigPin1, int echoPin1, int trigPin2, int echoPin2, int trigPin3, int echoPin3, int pulseWidth, int pulseSchedule) {
    _trigPin1 = trigPin1;
    _echoPin1 = echoPin1;
    _trigPin2 = trigPin2;
    _echoPin2 = echoPin2;
    _trigPin3 = trigPin3;
    _echoPin3 = echoPin3;

    pinMode(trigPin1, OUTPUT);
    digitalWrite(trigPin1, LOW);
    pinMode(trigPin2, OUTPUT);
    digitalWrite(trigPin2, LOW);
    pinMode(trigPin3, OUTPUT);
    digitalWrite(trigPin3, LOW);

    pinMode(echoPin1, INPUT);
    pinMode(echoPin2, INPUT);
    pinMode(echoPin3, INPUT);

    pulse_schedule = pulseSchedule;
    trigger_time_count = pulse_schedule;
    
    timer_setup();
    attachInterrupt(digitalPinToInterrupt(echoPin1), echo_interrupt_left, CHANGE);
    attachInterrupt(digitalPinToInterrupt(echoPin2), echo_interrupt_center, CHANGE);
    attachInterrupt(digitalPinToInterrupt(echoPin3), echo_interrupt_right, CHANGE);
}

void timer_setup() {
  // Write TOP:
  TCB2.CCMP =  2500; // Overflow after 10 ms using the TCA clk 
  TCB2.INTCTRL = TCB_CAPT_bm;
  TCB2.CTRLB = TCB_CNTMODE_INT_gc; // Setup timer into periodic interupt mode
  TCB2.CTRLA = TCB_ENABLE_bm | TCB_CLKSEL_CLKTCA_gc; // Enable counter Using the TCA clock
}

ISR(TCB2_INT_vect) {
  trigger_pulse();
  
  TCB2.INTFLAGS = TCB_CAPT_bm;
}

// --------------------------
// trigger_pulse() called every 50 uS to schedule trigger pulses.
// Generates a pulse one timer tick long.
// Minimum trigger pulse width for the HC-SR04 is 10 us. This system
// delivers a 50 uS pulse.
// --------------------------
void trigger_pulse() {
  static int schedule = 0;
  
  if ((schedule++) == pulse_schedule){
    digitalWrite(_trigPin1, HIGH);
    digitalWrite(_trigPin2, HIGH);
    digitalWrite(_trigPin3, HIGH);
  
    delayMicroseconds(10);
    
    digitalWrite(_trigPin1, LOW);
    digitalWrite(_trigPin2, LOW);
    digitalWrite(_trigPin3, LOW);

    schedule = 0;
  }
}

// --------------------------
// echo_interrupt() External interrupt from HC-SR04 echo signal. 
// Called every time the echo signal changes state.
//
// Note: this routine does not handle the case where the timer
//       counter overflows which will result in the occassional error.
// --------------------------
void echo_interrupt_left() {
  switch (digitalRead(_echoPin1)) {
    case HIGH:
      _echo_end_left = 0;
      _echo_start_left = micros();
      break;
      
    case LOW:
      _echo_end_left = micros();
      leftFilter.filter((double)(_echo_end_left - _echo_start_left));
      break;
  }
}

void echo_interrupt_center() {
  switch (digitalRead(_echoPin2)) {
    case HIGH:
      _echo_end_center = 0;
      _echo_start_center = micros();
      break;
      
    case LOW:
      _echo_end_center = micros();
      centerFilter.filter((double)(_echo_end_center - _echo_start_center));
      break;
  }
}

void echo_interrupt_right() {
  switch (digitalRead(_echoPin3)) {
    case HIGH:
      _echo_end_right = 0;
      _echo_start_right = micros();
      break;
      
    case LOW:
      _echo_end_right = micros();
      rightFilter.filter((double)(_echo_end_right - _echo_start_right));
      break;
  }
}

double Ultrasound::getDistance(int ultrasonicNum) {
    long duration;

    switch (ultrasonicNum) {
      case 1:
        duration = leftFilter.output;
        break;
        
      case 2:
        duration = centerFilter.output;
        break;
        
      case 3:
        duration = rightFilter.output;
        break;
        
      default:
        duration = -1;
        break;
    }
    
    return microsecondsToCentimeters(duration);
}

double Ultrasound::microsecondsToInches(long microseconds) {
    // According to Parallax's datasheet for the PING))), there are 73.746
    // microseconds per inch (i.e. sound travels at 1130 feet per second).
    // This gives the distance travelled by the ping, outbound and return,
    // so we divide by 2 to get the distance of the obstacle.
    // See: http://www.parallax.com/dl/docs/prod/acc/28015-PING-v1.3.pdf
    return ((double) microseconds) / 74.647 / 2.0;
}

double Ultrasound::microsecondsToCentimeters(long microseconds) {
    // The speed of sound is 340 m/s or 29 microseconds per centimeter.
    // The ping travels out and back, so to find the distance of the object we
    // take half of the distance travelled.
    return ((double) microseconds) / 29.388 / 2.0;
}
