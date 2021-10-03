# Serialtext Programm

Verwendung der seriellen Schnittstelle für Text-Übertragung.

Entwicklungsumgebung:
- Xunbuntu (Xfce + Ubuntu)
- Editor, Sublime Text
- gcc-avr : GNU C und C++ Cross-Compiler für AVR Zielsystem
- avr-libc : Standard C Bibliothek für den AVR
- binutils-avr : Hilfsprogramme für Handhabung von AVR Binärdateien
- avrdude : Programmiersoftware für AVR, auch Arduino Board
- Arduino UNO mit dem Atmega 328P

## Ziele

- Text Nachrichten empfangen (bis 256 Zeichen, CR terminiert)
- Text Nachrichten senden
- Ringpuffer für Senden/Empfang
- Warteliste für Nachrichten
- Periodische Heartbeat Nachricht
- Echo der empfangenen Nachrichten
- onboard LED umschalten über Nachricht "LED"

Gewählte Parameter für die serielle Schnittstelle:
57600 Baud, 8 Datenbits, Even-Parity, 1 Stopbit.

Verwendet wird der USART0 des Controllers.
Auf dem Arduino UNO Board sind die Tx/Rx Anschlüsse des USART0
über den Schnittstellenkonverter mit dem USB-Anschluss verbunden.

## Implementierung

C Source compilieren:

```bash
CFLAGS="-mmcu=atmega328p -std=gnu99 -Os -Wall -save-temps"
avr-gcc $CFLAGS -o main.elf serialtext.c
```

Optimiert wird das Programm auf minimale Größe (-Os).
Der erzeugte Assemblercode kann direkt kontrolliert werden,
durch -save-temps werden die Zwischenstufen nicht gelöscht.

Übertragen des Programms auf den Arduino:

```bash
avr-objcopy -O ihex main.elf main.hex
avrdude -v -c arduino -P /dev/ttyACM0 -p m328p -D -U flash:w:main.hex
```

Beim Flashen mit avrdude die Option -D (disable auto erase for flash) setzen,
damit der Bootloader nicht gelöscht wird.

Die Datenübertragung ist sichtbar durch die beiden LEDs auf dem Arduino Board.
Eine Tx und eine Rx LED blinken bei der Übertragung der Daten.

Kommunikation mit dem Arduino über eine Terminal-Emulation,
tio - a simple TTY terminal I/O application.

```bash
tio --baudrate 57600 --parity even --local-echo /dev/ttyACM0
```

Parameter für tio:
- Baudrate 57600 passend zur Konfiguration im Programm
- Parity-Bit even
- Lokale Echo, weil das Programm kein Echo der einzelnen Zeichen liefert
- Das Gerät ist der USB-Seriell-Konverter auf dem Arduino
- Standardwerte: 8 Datenbits, 1 Stopbit

Beim Start des Terminals wird die serielle Schnittstelle eingeschaltet.
Das DTR Signal von der seriellen Schnittstelle führt zum Reset des Mikrocontrollers.

## Code-Analyse

```bash
avr-objdump -xd main.elf
```

## Referenzen

- [ATmega 328P Datasheet](https://ww1.microchip.com/downloads/en/DeviceDoc/Atmel-7810-Automotive-Microcontrollers-ATmega328P_Datasheet.pdf)

- [avr-libc Library Reference](https://www.nongnu.org/avr-libc/user-manual/modules.html)

- [GCC version 5.4.0](https://gcc.gnu.org/onlinedocs/gcc-5.4.0/gcc/), the current version of the avr-gcc
