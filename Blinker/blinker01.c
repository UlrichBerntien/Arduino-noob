/*
 * Blinker Program.
 * Blink onboard LED on Arduino UNO.
 * Timing by busy-wait delay loops.
 *
 * Created: Ulrich Berntien 2021-09-13
 */


// Arduino UNO: 16 MHz
#define F_CPU 16000000

#include <avr/io.h>
#include <util/delay.h>


int main (void)
{
    // set data direction
    // port B bit 5 is the Adruino onboard LED
    DDRB |= 1 << PB5;

    // endless blink loop
    // two short flashes each 2nd second
    while(1)
    {
        // LED 100 ms on, only a flash
        PORTB |= 1 << PB5;
        _delay_ms( 100 );
        // LED 400 ms off
        PORTB &= ~ (1 << PB5);
        _delay_ms( 400 );
        // LED 100 ms on, a second a flash
        PORTB |= 1 << PB5;
        _delay_ms( 100 );
        // LED 1400 ms off
        PORTB &= ~ (1 << PB5);
        _delay_ms( 1400 );
    }

    // program never returns
    return 0;
}
