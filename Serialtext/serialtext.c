/*
 * Text messages over serial interface.
 *
 * Hardware: Arduino UNO with ATmega 328P
 * Serial parameter: 57600 baud, 8 data bits, parity even, 1 stop bit.
 *
 * Created: Ulrich Berntien 2021-09-24
 */


// Arduino UNO: 16 MHz
#define F_CPU 16000000

// The serial port should use 57600 baud
#define BAUD 57600
// Calculate the values by the helper macros.
#include <util/setbaud.h>

#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/power.h>
#include <avr/sleep.h>
#include <stdlib.h>
#include <string.h>


// 1-second timer.
// Cycles after 255 seconds, around 4 minutes, back to 0.
volatile static uint8_t oneSecondCounter;


// 16-Bit Pointer and access to the bytes of the pointer.
typedef union
{
    char *p;
    // a[0] = low byte of the 16-bit pointer.
    uint8_t a[2];
} ptr_t;


// Read and write ring buffer data structure.
// The buffer is aligned to 256. So the lower byte of the address data[0] is 0.
typedef struct
{
    // The buffer content.
    char data[256] __attribute__((aligned(256)));
    // Number of chars in the buffer.
    uint8_t count;
    // Read the next char here.
    ptr_t read;
    // Write the next char here.
    ptr_t write;
} ring_t;


// Error counters.
static volatile uint8_t parityErrors;
static volatile uint8_t overflowErrors;

// End of input message.
static const char endOfMessage = '\r';


// Count number of messages in input buffer.
// A message is a sequence of characters with CR terminator.
static volatile uint8_t inputMessages;

// Serial communication input ring buffer.
static ring_t input;


// Serial communication output ring buffer.
static ring_t output;


// Handle the data register empty interrupt of USART0.
// Send next character if available.
ISR(USART_UDRE_vect)
{
    // Send buffer should be empty inside this ISR.
    // But check it to ensure no data will be overwritten.
    if (UCSR0A & (1 << UDRE0))
    {
        if (output.count > 0)
        {
            // Data could be send.
            UDR0 = *output.read.p;
            ++output.read.a[0]; // automatic 255 -> 0 at end of buffer.
            --output.count;
        }
    }
    // Disable interrupt if no data is in the send-buffer.
    if (output.count == 0)
    {
        // Clear empty transmit buffer interrupt enable bit.
        UCSR0B &= ~(1 << UDRIE0);
    }
}


// Handle the receive complete interrupt of USART0.
// Store character in buffer.
ISR(USART_RX_vect)
{
    if (UCSR0A & (1 << DOR0))
    {
        // There was a data overrun in the USART FIFO.
        ++overflowErrors;
    }
    while (UCSR0A & (1 << RXC0))
    {
        // Check parity error of the current byte
        const uint8_t pe = UCSR0A & (1 << UPE0);
        // Receive complete; read the byte from the USART FIFO.
        const char tmp = UDR0;
        if (pe)
        {
            // There was a parity error in the current byte
            ++parityErrors;
        }
        else if (input.count < 254 || tmp == endOfMessage)
        {
            // Store the byte in the buffer.
            *input.write.p = tmp;
            ++input.write.a[0];
            ++input.count;
            if (tmp == endOfMessage)
            {
                // end of message received
                ++inputMessages;
            }
        }
        else
        {
            // Buffer overflow: Ignore new data.
            // The last byte, index 255, is reserved for a endOfMessage char.
            ++overflowErrors;
        }
    }
}


// Timer/Counter 1 compare match A.
// This interrupt is called every second.
ISR(TIMER1_COMPA_vect)
{
    // Count the seconds.
    // The main loop needs the time data.
    ++oneSecondCounter;
}


// Initialise the MCU.
// - config power safe
// - config sleep mode
// - config port B
// - config USARS0
// - config 16-bit timer/counter 1
static void initialise()
{
    // Disable interrupts.
    cli();
    // Power off all unused MCU modules.
    power_timer0_disable();
    power_timer2_disable();
    power_twi_disable();
    power_spi_disable();
    power_adc_disable();
    // Ensure the used MCU modules are powered on.
    power_usart0_enable();
    power_timer1_enable();

    // Configure the sleep in IDLE mode.
    set_sleep_mode(SLEEP_MODE_IDLE);

    // Onboard LED is connected to port B pin 5.
    DDRB = 1 << DDB5;

    // Initialise the ring buffers for the serial communication.
    output.read.p = output.data;
    output.write.p = output.data;
    output.count = 0;
    input.read.p = input.data;
    input.write.p = input.data;
    input.count = 0;
    inputMessages = 0;
    parityErrors = 0;
    overflowErrors = 0;

    // First: Enable receiver (RXEN0) and transmitter (TXEN0) of the USART0.
    UCSR0B = (1 << RXEN0) | (1 << TXEN0);
    // Then: Configure the USART0 for serial communication.
    // Set the parameter for BAUD in UBRR0L and UBRR0H.
    UBRR0 = UBRR_VALUE;
#if USE_2X
    UCSR0A |= 1 << U2X0;
#else
    UCSR0A &= ~(1 << U2X0);
#endif
    // Set asynchronous mode (UMSEL01:0 = 0)
    // Set frame format to: 8 data bits (UCSZ00:2 = 3)
    //      and 1 stop bit (USBS0 = 0)
    //      and even parity (UPM00:1 = 2).
    UCSR0C = (1 << UPM01) | (1 << UCSZ01) | (1 << UCSZ00);
    // Enable receive complete interrupt (RXCIE0)
    UCSR0B |= (1 << RXCIE0);

    // Configure the Timer1 to generate a 1 second heartbeat.
    // Set clear timer on compare match (CTC) mode with TOP = OCR1A.
    // Set clock source to IO-clock/1024
    TCCR1A =  0;
    TCCR1B = (1 << WGM12) | (1 << CS12) | (1 << CS10);
    // 1 match interrupt per second.
    OCR1A = F_CPU / 1024 - 1;
    // Enable timer/counter 1 output compare match A interrupt.
    TIMSK1 = 1 << OCIE1A;
    // Start with second 0.
    oneSecondCounter = 0;

    // Allow interrupts.
    sei();
}


// Switch on the onboard LED.
static void LED_on(void)
{
    // onboard LED connected to port B pin 5.
    PORTB = 1 << PB5;
}


// Toggle the onboard LED.
static void LED_toggle(void)
{
    // use PINB register to read the current output status.
    PORTB = PINB ^ (1 << PB5);
}


// Enables the serial transmission.
static void enableSerialTransmission(void)
{
    // Enables the interrupt on data register empty.
    UCSR0B |= 1 << UDRIE0;
}


// Stores string (\0 terminated) in the output buffer to send.
// The function BLOCKS if the output buffer is full.
static void send(const char str[])
{
    char tmp;
    while ((tmp = *str++) != 0)
    {
        if (output.count < 255)
        {
            // buffer have capacity
            *output.write.p = tmp;
            ++output.write.a[0];
            ++output.count;
        }
        else
        {
            // Ensure transmission via ISR is enabled.
            enableSerialTransmission();
            // Buffer full. Sleep; wait for characters are sent.
            sleep_mode();
        }
    }
    // Ensure transmission via IRS is enabled.
    // A possible situation:
    //      The ISR sends the buffer content already.
    //      Then the ISR will disable the interrupt again.
    //      No need to check this rare situation here.
    // Enable empty transmission buffer interrupt.
    enableSerialTransmission();
}


// Reads a message from the serial communication input buffer.
// Stores the string in str. Length of the buffer is len.
// Returns empty string if no message is in the input buffer.
static void read(char str[], uint8_t len)
{
    if (inputMessages)
    {
        while (input.count && len)
        {
            const char tmp = *input.read.p;
            ++input.read.a[0];
            --input.count;
            if (tmp == endOfMessage)
            {
                // End of message reached.
                --inputMessages;
                break;
            }
            if (tmp > 0x1F && tmp != 0x7F)
            {
                // Store the message in the buffer.
                // Ignores all control characters like \n.
                *str++ = tmp;
                --len;
            }
        }
    }
    if (len == 0)
    {
        // Message buffer full. Ignore last char.
        --str;
    }
    *str = 0;
}


// Sends status information.
static void sendStatus(void)
{
    char buffer[8];
    // Current time, seconds, modulo 0xFF.
    send("STAT> sec: ");
    utoa(oneSecondCounter, buffer, 10);
    send(buffer);
    // Number of waiting messages.
    send(" msg.wait: ");
    utoa(inputMessages, buffer, 10);
    send(buffer);
    // Number of parity errors, modulo 0xFF.
    send(" parity.err: ");
    utoa(parityErrors, buffer, 10);
    send(buffer);
    // Number if overflow errors, modulo 0xFF
    send(" over.err: ");
    utoa(overflowErrors, buffer, 10);
    send(buffer);
    send("\r\n");
}


int main(void)
{
    // buffer to work with the messages inside the main function.
    char buffer[255];
    // Initalise board.
    initialise();
    // Give first life signal by onboard LED and message.
    LED_on();
    send("Hello\r\n");
    // Time of last message send.
    uint8_t lastSendTime = oneSecondCounter;
    // Work in an endless loop.
    while (1)
    {
        // Sleep until an message or timer event must be handled.
        sleep_mode();
        // Priority 1: Process received messages.
        while (inputMessages)
        {
            read(buffer, sizeof buffer);
            if (strcmp(buffer, "LED") == 0)
            {
                LED_toggle();
            }
            // Send echo to the
            send("ECHO> ");
            send(buffer);
            send("\r\n");
        }
        // Priority 2: Send status info as heartbeat.
        if (oneSecondCounter != lastSendTime)
        {
            sendStatus();
            lastSendTime = oneSecondCounter;
        }
        // Priority 3: Clear input buffer if filled with a too long message
    }

    // The process loop never terminates.
    return 0;
}
