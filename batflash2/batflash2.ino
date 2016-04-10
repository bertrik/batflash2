/*
  BatFlash
  (c) Bertrik Sikken, 2010
  
  Detects a flash using the flash slave trigger on in the PIN_FLASH input,
  then sends out an RF signal to a Hama CA-1 remote control attached to a camera to start a new exposure.
  Exposures are also triggered after a certain interval, controlled by the potmeter.
  
 */

#include <avr/sleep.h>
#include <avr/wdt.h>

#define CA1_BIT_PERIOD 119  // microseconds

// digital pins
const int PIN_FLASH = 2;
const int PIN_S1 = 7;
const int PIN_S2 = 6;
const int PIN_S3 = 5;
const int PIN_S4 = 4;
const int PIN_RF = 12;

const int PIN_LED1 = 8;
const int PIN_LED2 = 9;

// analog pins
const int PIN_POT = 0;


// variables
static volatile boolean fTrigger = false;
static long time = 0;

/**
 * Delay for the specified time for the CA-1 protocol
 *
 * @param d time units to delay
 */
static void ca1_delayT(int d)
{
  delayMicroseconds(d * CA1_BIT_PERIOD);
}

/**
 * Send a code according to the Hama CA-1 protocol
 * 
 * @param channel the radio channel (0-15) to send on
 * @param fullpress when true, send fullpress code, otherwise halfpress
 */
static int ca1_send(int repeat, unsigned char channel, bool fullpress)
{
  unsigned char data;
  int n, i;

  // calculate variable part over the air
  data = ~channel & 0x0F;
  if (fullpress) {
    // special case for code 15
    if (channel == 15) {
      data |= 0xF0;
    }
    else {
      data |= (data << 4);
    }
  }

  // repeat x times
  for (n = 0; n < repeat; n++) {

    // send 8 data bits
    for (i = 0; i < 8; i++) {
      digitalWrite(PIN_RF, 1);
      ca1_delayT(2);
      digitalWrite(PIN_RF, 0);
      if (data & (0x80 >> i)) {
        // 1 bit
        ca1_delayT(6);
      }
      else {
        ca1_delayT(2);
      }
    }

    // send epilogue
    digitalWrite(PIN_RF, 1);
    ca1_delayT(1);
    digitalWrite(PIN_RF, 0);
    ca1_delayT(2);
    digitalWrite(PIN_RF, 1);
    ca1_delayT(4);
    digitalWrite(PIN_RF, 0);
    ca1_delayT(16);
  }
}

/**
 * Return the current channel code as configured by the DIP switches
 *
 * @return the channel code
 */
static unsigned char getCode() {
  static const int pins[] = {
    PIN_S1, PIN_S2, PIN_S3, PIN_S4  };
  int i;
  unsigned char code;

  code = 0;
  for (i = 0; i < 4; i++) {
    code <<= 1;
    if (digitalRead(pins[i]) == LOW) {
      code |= 1;
    }
  }

  return code;
}


// 0=16ms, 1=32ms,2=64ms,3=128ms,4=250ms,5=500ms
// 6=1 sec,7=2 sec, 8=4 sec, 9= 8sec
static void setup_watchdog(int ii) 
{
  byte bb;

  bb = ((ii & 8) << 2) | (ii & 7);

  // clear watchdog reset
  MCUSR &= ~(1<<WDRF);
  // start timed sequence
  WDTCSR |= (1<<WDCE) | (1<<WDE);
  // set new watchdog timeout value
  WDTCSR = bb | (1 << WDCE);
  WDTCSR |= _BV(WDIE);
}

// set system into the sleep state 
// system wakes up when wtchdog is timed out
static void system_sleep() 
{
  // switch Analog to Digitalconverter OFF
  ADCSRA |= ADEN;

  // sleep mode is set here
  set_sleep_mode(SLEEP_MODE_STANDBY);
  sleep_enable();
  // System sleeps here
  sleep_mode();                        
  // System continues execution here when watchdog timed out 
  sleep_disable();                   

  // switch Analog to Digitalconverter ON  
  ADCSRA &= ~ADEN;
}

/* interrupt routine for external wake-up */
static void wakeUp(void)
{
  fTrigger = true;
  detachInterrupt(0);
}

// Watchdog Interrupt Service / is executed when  watchdog timed out
ISR(WDT_vect) 
{
  // dummy, do nothing
}

/**
 * Reset the flash trigger
 */
static void resetTrigger()
{
  time = 0;
  fTrigger = false;

  attachInterrupt(0, wakeUp, LOW);
}

static void start_exposure()
{
  int i;
  unsigned char channel = getCode();

  digitalWrite(PIN_LED2, 1);
  for (i = 0; i < 16; i++) {
    if (fTrigger) {
      break;
    }
    ca1_send(16, channel, true);
  }
  digitalWrite(PIN_LED2, 0);
}

static void stop_exposure()
{
  unsigned char channel = getCode();

  digitalWrite(PIN_LED1, 1);
  ca1_send(20, channel, false);
  digitalWrite(PIN_LED1, 0);
}

/**
 * Setup function
 */
void setup() {
  int i;
  
  // enable pull-ups on dip-switch inputs
  digitalWrite(PIN_S1, 1);
  digitalWrite(PIN_S2, 1);
  digitalWrite(PIN_S3, 1);
  digitalWrite(PIN_S4, 1);

  // LEDs
  pinMode(PIN_LED1, OUTPUT);
  pinMode(PIN_LED2, OUTPUT);
  for (i = 0; i < 5; i++) {
    digitalWrite(PIN_LED1, 1);
    delay(100);
    digitalWrite(PIN_LED1, 0);
    delay(100);
    digitalWrite(PIN_LED2, 1);
    delay(100);
    digitalWrite(PIN_LED2, 0);
    delay(100);
  }

  // configure RF output
  pinMode(PIN_RF, OUTPUT);

  // enable pull-up on flash input
  digitalWrite(PIN_FLASH, 1);

  // setup watchdog for 1 second delay
  setup_watchdog(6);
  
  // arm flash trigger
  resetTrigger();

  // start a new exposure
  start_exposure();
}

/**
 * Loop function
 */
void loop()
{
  // sleep for 1 second (waking up immediately when a flash is detected)
  if (!fTrigger) {
    // sleep for a second
    system_sleep();
    time++;
    
    // show 1-second pulse
    digitalWrite(PIN_LED1, 1);
    delay(2);

    // read pot meter and check if interval has been reached
    int potValue = analogRead(PIN_POT);
    long interval = map(potValue, 0, 1023, 30, 600);
    if (time >= interval) {
      fTrigger = true;
    }
    
    // clear 1-second pulse LED
    digitalWrite(PIN_LED1, 0);
  }

  // check if we need a new trigger
  if (fTrigger) {
    // stop exposure
    stop_exposure();
    
    // wait a bit
    resetTrigger();
    delay(100);
    
    // start new exposure
    start_exposure();
  }

}

