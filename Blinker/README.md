# Blinker Programm

Die typische "Hello World" Programmierübung auf dem Arduino:
LED blinken lassen.


Verschiedene Entwicklungsumgebungen stehen für die AVR Mikrocontroller zur Verfügung.

Hier wird verwendet: GNU C Toolchain auf Xunbuntu (Xfce + Ubuntu).
Dazu ein Editor, der Sublime Text.

Die einfache Toolchain ohne eine umfangreiche IDE soll den Blick auf die
Grundlagen erlauben.


- gcc-avr : GNU C und C++ Cross-Compiler für AVR Zielsystem
- avr-libc : Standard C bibliothek für den AVR
- binutils-avr : Hilfsprogramme für Handhabung von AVR Binärdateien
- avrdude : Programmiersoftware für AVR, auch Arduino Board


```bash
apt install gcc-avr avr-libc binutils-avr avrdude
```

## Blinker nach AVR-GCC-Tutorial

Blinken der onboard LED verbunden mit Pin 13 des Controllers.

C Sourcecode-Datei: [blinker01.c](blinker01.c)

### Compileren und Arduino flashen

C Source compilieren:

```bash
CFLAGS="-mmcu=atmega328 -Os -Wall -std=gnu99"
avr-gcc $CFLAGS -o main.elf blinker01.c
```

Compiler Optionen:

- -mmcu=atmega328 : Auswahl des Ziels, Name des MCU
- -std=gnu99 : Sprachversion C99 mit GNU Erweiterungen auswählen
- -Os : Optimieren auf geringe Code-Größe
- -Wall : Viele Warnungen ausgeben
- -o main.elf : Ausgabedatei des Programms

Erzeugen der Datei im Intel-Hex Format für das Programmieren des Controllers:

```bash
avr-objcopy -O ihex main.elf main.hex
```

Verarbeitungs Optionen:

- -I elf32-avr : das Eingabeformat wird automatisch erkannt; muss nicht angegeben werden
- -O ihex : das Ausgabeformat ist Intel-Hex

Übertragen des Programms auf den Arduino.
Das Programm wird nach dem Schreiben ausgeführt.

```bash
avrdude -v -c arduino -P /dev/ttyACM0 -p m328p -D -U flash:w:main.hex
```

Optionen für den avrdude:

- -v : Mehr Ausgaben über die laufende Arbeit
- -c ardunio : Programmiergerät ist ein Arduino
- -p m328p : Programmiert wird ein ATmega 328P
- -P /dev/ttyACM0 : die Schnittstelle zum Controller hat den Namen /dev/ttyACM0,
  die Standardbaudrate ist 115200
- -D : Flash nicht löschen (ohne -D wird der Bootloader gelöscht)
- -U flash:w:main.hex : Schreiben in den Flash-Speicher aus der Datei main.hex,
  dabei wird der Dateityp automatisch erkannt.

### Der erzeugte Code

Ansehen des C-Codes nach Preprocessor (blinker01.i) und des Assemblercodes (blinker01.s):

```bash
CFLAGS="-mmcu=atmega328 -Os -Wall -std=gnu99"
avr-gcc -save-temps $CFLAGS -o main.elf blinker01.c
```

Ansehen des erzeugten Codes mit Disassembler Ausgabe:

```bash
avr-objdump -xd main.elf
```

Objectdump Optionen:

- -x : Metadaten ausgegeben
- -d : Assemblercodes der ausführbaren Abschnitte ausgehen

Der Code beginnt bei der Adresse 0.

```
Disassembly of section .text:

00000000 <__vectors>:
   0:   0c 94 34 00     jmp 0x68    ; 0x68 <__ctors_end>
   4:   0c 94 3e 00     jmp 0x7c    ; 0x7c <__bad_interrupt>
   8:   0c 94 3e 00     jmp 0x7c    ; 0x7c <__bad_interrupt>
 [...]
  5c:   0c 94 3e 00     jmp 0x7c    ; 0x7c <__bad_interrupt>
  60:   0c 94 3e 00     jmp 0x7c    ; 0x7c <__bad_interrupt>
  64:   0c 94 3e 00     jmp 0x7c    ; 0x7c <__bad_interrupt>
```

Am Anfang steht die Interrupt Tabelle mit 26 Interrupt Vektoren.
Die Vektoren enthalten einen alle eine Jump Anweisung.

Der Reset Interrupt führt auf die Initialisierung des Programms
\__ctors_end und dort weiter zur programmierten main Funktion.

Alle anderen Interrupts führen auf:

```
0000007c <__bad_interrupt>:
  7c: 0c 94 00 00   jmp 0 ; 0x0 <__vectors>
```

Durch den Sprung `jmp 0` werden alle Interrupts wie Reset behandelt.


Der Compiler bindet Code für die Initialisierung vor dem Aufruf von main ein:

```
00000068 <__ctors_end>:
  68:   11 24           eor r1, r1
  6a:   1f be           out 0x3f, r1    ; 63
  6c:   cf ef           ldi r28, 0xFF   ; 255
  6e:   d8 e0           ldi r29, 0x08   ; 8
  70:   de bf           out 0x3e, r29   ; 62
  72:   cd bf           out 0x3d, r28   ; 61
  74:   0e 94 40 00     call 0x80       ; 0x80 <main>
  78:   0c 94 6a 00     jmp 0xd4        ; 0xd4 <_exit>
```

Das Statusregister (SREG), Register 0x3F, wird gelöscht.
Alle Bits werden auf 0 gesetzt, weil r1 nach `eor r1,r1` (Exclusive or) den Wert 0 hat.

Register 0x3E (SPH) und 0x3D (SPL) bilden den 11-bit Stackpointer (SP).
Der Stack wird initialisiert auf die oberste Adresse 0x8FF des Arbeitsspeichers (SRAM).

Nach der Initialisierung wird die programmierte main Funktion aufgerufen `call 0x80`.
Sollte die main Funktion beendet werden (`ret`), wird in den \_exit Code gesprungen

Der Anfang der main Funktion:

```
00000080 <main>:
  80:   25 9a           sbi 0x04, 5         ; 4
  82:   2d 9a           sbi 0x05, 5         ; 5
  84:   2f ef           ldi r18, 0xFF       ; 255
  86:   81 ee           ldi r24, 0xE1       ; 225
  88:   94 e0           ldi r25, 0x04       ; 4
  8a:   21 50           subi    r18, 0x01   ; 1
  8c:   80 40           sbci    r24, 0x00   ; 0
  8e:   90 40           sbci    r25, 0x00   ; 0
  90:   e1 f7           brne    .-8         ; 0x8a <main+0xa>
  92:   00 c0           rjmp    .+0         ; 0x94 <main+0x14>
  94:   00 00           nop
```

Zuerst wird mit `sbi 0x04, 5` (set bit in I/O register)
der Anschluss Port B5 auf Ausgabe geschaltet (DDB1 = 1).
(Register 0x04 = Port B data direction register)

Mit `sbi 0x05, 5` wird der Port B5 (PORTB5) auf High geschaltet.
Damit leuchtet die Onboard LED.
(Register 0x05 = Port B data register)

Es folgt die programmierte Verzögerung um 100ms.
Die drei Register r18, r24, r25 werden für einen 24-Bit Zähler verwendet.
Der Startwert 0x04E1FF = 319999 wurde durch Makros aus Zeitvorgabe und CPU Takt berechnet.
`sbci` (subtract immediate with carry) verknüpft die Subtraktionen der Bytes
durch das Carry-Flag und das Zero-Flag.
Die Subtraktion `sbci` löscht das Zero-Flag bei einem Ergebnis != 0, setzt das Flag aber nie.
Die Schleife läuft solange nicht 0 erreicht ist `brne` (branch if not equal, not equal = not zero).
Der Sprung zur direkt folgenden Anweisung `rjmp .+0` und `nop` haben keine erkennbare Funktion,
vermutlich entstanden durch nicht perfekte Optimierung durch den Compiler.

Es folgen weitere Blöcke "LED schalten, aktive Verzögerung".
Am Ende der main Funktion wird wieder zum Anfang der while Schleife gesprungen.

Der Compiler hat zwei weitere Funktionen erzeugt:

```
000000d4 <_exit>:
  d4:   f8 94           cli

000000d6 <__stop_program>:
  d6:   ff cf           rjmp    .-2         ; 0xd6 <__stop_program>
```

Alle Interrupts werden ausgeschaltet, `cli` (Clear global interrupt flag).

Der Sprung `rjmp .-2` (Relative jump) zurück auf seine Adresse ergibt eine Endlosschleife.

Über die \_exit Funktion wird der Programmfluss gestoppt, falls die main Funktion beendet wird.


## Quellen

- [RN-Wissen: avr-gcc](https://rn-wissen.de/wiki/index.php/Avr-gcc)

- [AVR Libc Home Page](https://www.nongnu.org/avr-libc/user-manual/index.html)

- [mikrocontroller.de: AVR-GCC-Tutorial](https://www.mikrocontroller.net/articles/AVR-GCC-Tutorial)

- [mikrocontroller.de: AVR-GCC](https://www.mikrocontroller.net/articles/AVR-GCC)

- [ATmega 328P Datasheet](https://ww1.microchip.com/downloads/en/DeviceDoc/Atmel-7810-Automotive-Microcontrollers-ATmega328P_Datasheet.pdf)

- [AVR Instruction Set Manual](http://ww1.microchip.com/downloads/en/devicedoc/atmel-0856-avr-instruction-set-manual.pdf)
