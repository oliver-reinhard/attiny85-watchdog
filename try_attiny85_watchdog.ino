/*
 * Demonstrates the use of ATtiny85 watchdog interrupts to wake up MCU from sleep periodically  
 * to do some useful work while preserving energy most of the time. After some time the watchdog
 * is disabled, leaving the MCU sleep forever.
 * 
 * Uses an LED on PB0.
 * 
 * Expected behaviour:
 * 
 * - LED lights up solid for 3 seconds, then:
 * - Watchdog goes off every 1 second and triggers an interrupt --> 100 ms blink
 * - After 5th interrupt --> 3 blinks of 100 ms
 * - After the third time:
 *    - three more single blinks after a 1 second
 *    - then LED solid for 3 seconds
 *    - then silence forever (watchdog disabled --> no more interrupts, MCU sleeps, no wakeup)
 *    
 * Demonstrates two ways of configuring watchdog interrupts:
 * 
 * - Option A: via wdt_enable(..), which sets WDE = 1, and resetting WDIE = 1 after each "last-chance" interrupt
 * - Option B: via a custom procedure that sets WDE = 0

 * Option A is independent of WDT control-register name which varies across AVR processors.
 * 
 * Make use of #define OPTION_A_USE_WDT_ENABLE to choose option
 * 
 * Oliver Reinhard, 2022
 * 
 * Special thanks to Wolles Elektronikkiste getting me on the right track in his 
 * post https://wolles-elektronikkiste.de/en/sleep-modes-and-power-management#Anker4
 * 
 */
#include <avr/sleep.h>
#include <avr/wdt.h>

#define OPTION_A_USE_WDT_ENABLE  // remove / comment this line for option B

const uint8_t FLASHING_LED_PIN = PB5;     // digital: out (LED — PB0 is the built-in on ATtiny85 programmer)

uint8_t watchdog_counter = 0;
uint8_t watchdog_disable_countdown = 3;

void setup() {
  pinMode(FLASHING_LED_PIN, OUTPUT);
  turnOnLED(3000);
  disable_watchdog();

  #ifdef OPTION_A_USE_WDT_ENABLE
    // Must reset WDIE after each interrupt!!
    wdt_enable(WDTO_1S);
    _WD_CONTROL_REG |= _BV(WDIE);
    sei();

  #else
    setup_watchdog(WDTO_1S);
  #endif
}

void loop() {
  //  sleep_mode(); // Blocking sleep ("idle" mode by default)
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_enable();
  sei();
  sleep_cpu();      // Controller waits for interrupt here
  sleep_disable();

  if(watchdog_counter >= 5)   {
    flashLED(3);
    watchdog_counter = 0;
    watchdog_disable_countdown--;
    if (watchdog_disable_countdown == 0) {
      delay(3500);          // LED should flash 3 times more
      disable_watchdog();   // Permanently disable --> MCU will sleep and never wake up again …
      turnOnLED(3000);
    }
  }
}


// This runs each time the watch dog wakes us up from sleep
ISR(WDT_vect) {
  watchdog_counter++;
  flashLED(1); 
  #ifdef OPTION_A_USE_WDT_ENABLE
    _WD_CONTROL_REG |= _BV(WDIE);
  #endif
}

 #ifndef OPTION_A_USE_WDT_ENABLE
  //
  // THIS IS OPTION B:
  //
  void setup_watchdog(uint8_t timeoutBitmask){
    cli();
    wdt_reset();                                                  // Reset counter to zero
    _WD_CONTROL_REG  = (1<<WDIE) | (0<<WDE) | timeoutBitmask;               // Enable interrupt, no system reset
    // _WD_CONTROL_REG  = (1<<WDIE) | (0<<WDE) | (1<<WDP2) | (1<<WDP1);     // Alternative, 1 second
    sei();
  }
#endif

void disable_watchdog() {
  MCUSR &= ~_BV(WDRF); // see comment in ATtiny85 Datasheet, p.46, Note under "Bit 3 – WDE: Watchdog Enable" 
  wdt_disable();
}

void turnOnLED(uint32_t ms) {
  digitalWrite(FLASHING_LED_PIN, HIGH);
  delay(ms);
  digitalWrite(FLASHING_LED_PIN, LOW);
}
  
void flashLED(uint8_t times) {
  for (uint8_t i = 0; i < times; i++) {
    digitalWrite(FLASHING_LED_PIN, HIGH);
    delay(100);
    digitalWrite(FLASHING_LED_PIN, LOW);
    delay(100);
  }
}
