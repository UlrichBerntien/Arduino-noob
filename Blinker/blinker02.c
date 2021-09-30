/*
 * Blinker Program, second version.
 *
 * Blink onboard LED on Arduino UNO.
 * Timing by 8-bit timer 0 and idle mode sleep.
 * Timer 0 is available because Arduino UNO board is used
 * but not the Arduino library.
 *
 * Created: Ulrich Berntien 2021-09-15
 */


// Arduino UNO: 16 MHz
#define F_CPU 16000000

#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/sleep.h>


// Counter of timer interrupts.
// This counter will be incremented each 10 ms.
// The counter will be reseted for each blink periode.
// In the program is only this global variable,
// it could be stored in one of the many registers.
register uint8_t counter asm("r2");


// Interrupt Service Routine for timer 0 compare A match.
// Called by the timer 0 each 10ms.
// ISR is a Macro to generate the function prototype.
ISR(TIMER0_COMPA_vect)
{
    counter++;
    // The following interrupt returns enables the interrupts
}


// Initialisation of the controller.
void initialisation(void)
{
    // no interrupts during initialisation
    // (No old status (SREG) save because it is the initialisation here.)
    cli();

    // Select idle sleep mode
    // In idle sleep mode the IO clock runs and timer 0 runs
    set_sleep_mode(SLEEP_MODE_IDLE);

    // Clear the 10ms counter.
    counter = 0;

    // set data direction of port B bit 5 as output (= 1)
    // and set all other port B bits as input (= 0)
    // PORTB5 is connected to the Arduino onboard LED
    DDRB = 1 << DDB5;

    // configure the timer to 10ms period
    // calculation: 16 MHz / 1024 / 156 = 100.16 Hz
    // time error less than 1% is acceptable here
    // set Output Compare Register A for timer 0 for div 156
    OCR0A = 156 - 1;
    // set timer 0 to CTC mode: Clear timer on compare match
    TCCR0A = 1 << WGM01;
    // prescaler div 1024 set in the Timer/Counter Control Register B TCCR
    TCCR0B = (1 << CS02) + (1 << CS00);
    // set Output Compare Interrupt Enable interrupt for timer 0 Match A
    // and disable all other timer 0 interrupts
    TIMSK0 = 1 << OCIE0A;

    // general allow interrupts, independent of previous SREG content
    sei();
}


// Table of light signals durations.
// Duration time in 10ms unit, duration 0 is end of list.
// 3 flashes with growing period time.
const uint8_t duration [] = {50, 50, 100, 100, 200, 200, 0};


int main(void)
{
    initialisation();

    uint8_t wait_until = 0;
    uint8_t next = 0;
    // endless loop: sleep, wakeup, work, sleep ...
    while (1)
    {
        // sleep until interrupt wakes
        sleep_mode();
        // wait until end of current duration
        if (counter >= wait_until)
        {
            // switch PORTB5, the onboard LED
            PORTB = PINB ^ (1 << PB5);
            next++;
            if (!duration[next])
            {
                // end of list reached: take first item.
                next = 0;
            }
            wait_until = duration[next];
            // Clear counter to measure the next duration.
            counter = 0;
        }
    }
    // Program never returns.
    return 0;
}
