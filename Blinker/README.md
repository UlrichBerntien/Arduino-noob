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


## Blinker mit aktiver Warteschleife

Programmiert nach [AVR-GCC-Tutorial](https://www.mikrocontroller.net/articles/AVR-GCC-Tutorial).

Blinken der onboard LED verbunden mit Pin 13 des Controllers.
Die Zeiten werden durch aktives Warten erzeugt.

C Sourcecode-Datei: [blinker01.c](blinker01.c)

### Compileren und Arduino flashen

C Source compilieren:

```bash
CFLAGS="-mmcu=atmega328p -Os -Wall -std=gnu99"
avr-gcc $CFLAGS -o main.elf blinker01.c
```

Compiler Optionen:

- -mmcu=atmega328p : Auswahl des Ziels, Name des MCU
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
CFLAGS="-mmcu=atmega328p -Os -Wall -std=gnu99"
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
Es ist eine Sprung-Liste.
Die Interrupt Adresse wird nicht gelesen, es wird in die Tabelle gesprungen.

Der Reset Interrupt führt auf die Initialisierung des Programms
\_ctors_end und dort weiter zur programmierten main Funktion.

Alle anderen Interrupts führen auf:

```
0000007c <__bad_interrupt>:
  7c: 0c 94 00 00   jmp 0 ; 0x0 <__vectors>
```

Durch den Sprung `jmp 0` werden alle Interrupts wie Reset behandelt.


Der Compiler bindet Code für die Initialisierung vor dem Aufruf von main ein:

```
00000068 <__ctors_end>:
  68:   11 24           eor r1, r1      ; r1 = 0
  6a:   1f be           out 0x3f, r1    ; SREg = 0
  6c:   cf ef           ldi r28, 0xFF
  6e:   d8 e0           ldi r29, 0x08   ; r29:28 = 0x08ff
  70:   de bf           out 0x3e, r29
  72:   cd bf           out 0x3d, r28   ; SP = 0x08ff
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
  80:   25 9a           sbi 0x04, 5
  82:   2d 9a           sbi 0x05, 5
  84:   2f ef           ldi r18, 0xFF       ; initialise 24 bit wait loop counter
  86:   81 ee           ldi r24, 0xE1
  88:   94 e0           ldi r25, 0x04
  8a:   21 50           subi    r18, 0x01   ; decrement 24 bit wait loop counter
  8c:   80 40           sbci    r24, 0x00
  8e:   90 40           sbci    r25, 0x00
  90:   e1 f7           brne    .-8         ; 0x8a <main+0xa>
  92:   00 c0           rjmp    .+0         ; 0x94 <main+0x14>
  94:   00 00           nop
```

Zuerst wird mit `sbi 0x04, 5` (set bit in I/O register)
der Anschluss Port B5 auf Ausgabe geschaltet (Data direction B bit 5, DDB5).
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
Die Schleife läuft so lange nicht 0 erreicht ist `brne` (branch if not equal, not equal = not zero).
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



## Blinker mit Ruhen und Timer Interrupts

Programmiert nach [AVR-GCC-Tutorial](https://www.mikrocontroller.net/articles/AVR-GCC-Tutorial/Die_Timer_und_Z%C3%A4hler_des_AVR).

Blinken der onboard LED verbunden mit Pin 13 des Controllers.
Die Zeiten werden durch den Timer 0 des Controllers erzeugt.
In den Pausen schläft der Controller im IDLE Modus.

Die Hardware des Arduino wird verwendet.
Die Arduino IDE mit ihrer Bibliothek wird nicht verwendet,
daher gibt es keine Kollisionen mit der Verwendung des Timer 0 in der Arduino Bibliothek.

C Sourcecode-Datei: [blinker02.c](blinker02.c)

### Compileren und Arduino flashen

Compileren und in Intel-Hex Format konvertieren.

```bash
CFLAGS="-mmcu=atmega328p -Os -Wall -std=gnu99"
avr-gcc -save-temps $CFLAGS -o main.elf blinker02.c
avr-objcopy -O ihex main.elf main.hex
```

Code in den Arduino programmieren.

```bash
avrdude -v -c arduino -P /dev/ttyACM0 -p m328p -D -U flash:w:main.hex
```

Das Programm startet nach dem Schreiben automatisch.

### Der erzeugte Code

Dump und Disassembler des erzeugten Codes:

```bash
avr-objdump -sd main.elf > main.out
```

Objectdump Optionen:

- -s : Inhalte als Hexdump ausgegeben
- -d : Assemblercodes der ausführbaren Abschnitte ausgehen

```
Contents of section .data:
 800100 32326464 c8c80000                    22dd....
```

Die statischen Daten sind im Abschnitt .data.
Das ist der Inhalt des Arrays mit dem Zeitdauern für das Ein/Ausschalten.

Der generierte Code ist im Abschnitt .text.

```
   0: 0c 94 34 00   jmp 0x68  ; 0x68 <__ctors_end>
   4: 0c 94 51 00   jmp 0xa2  ; 0xa2 <__bad_interrupt>
[...]
  34: 0c 94 51 00   jmp 0xa2  ; 0xa2 <__bad_interrupt>
  38: 0c 94 53 00   jmp 0xa6  ; 0xa6 <__vector_14>
  3c: 0c 94 51 00   jmp 0xa2  ; 0xa2 <__bad_interrupt>
[...]
  60: 0c 94 51 00   jmp 0xa2  ; 0xa2 <__bad_interrupt>
  64: 0c 94 51 00   jmp 0xa2  ; 0xa2 <__bad_interrupt>
```

Ab Adresse 0 beginnt die Interrupt Tabelle (Sprung Liste) mit 26 Einträgen.

Der Reset (0x00) startet die Initialisierung des Programms.

Der "Timer 0 compare match A" interrupt (Nummer 14) führt auf die programmierte Interrupt Service Routine.
Der Name der Funktion \_vector_14 wurde erzeugt durch das Makro ISR.
Alle anderen Interrupts führen auf \_bad_interrupt, die Reset aufruft.

Der Name der ISR für den Timer wurde vom Makro ISR erzeugt:

```
00000096 <__vector_14>:
  96: 1f 92         push  r1            ; save r1
  98: 0f 92         push  r0            ; save r0
  9a: 0f b6         in  r0, 0x3f
  9c: 0f 92         push  r0            ; save SREG = 0x3f
  9e: 11 24         eor r1, r1          ; r1 := 0
  a0: 23 94         inc r2              ; counter++
  a2: 0f 90         pop r0              ; restore registers
  a4: 0f be         out 0x3f, r0
  a6: 0f 90         pop r0
  a8: 1f 90         pop r1
  aa: 18 95         reti                ; return from IST (implicit enable interrupts)

```

Der erzeugte Code sichert alle verwendeten Register auf dem Stack.
Gesichert und wieder gestellt werden r1, r0 und das Status register (SREG, 0x3f).

Der 10ms Zähler ist im Register r2 gespeichert.
Der Zähler ist nicht im RAM abgelegt, existiert nur im Register r2.
Das Inkrement des Zählers ist die Funktion der Interrupt Service Routine (ISR).

Der vom Compiler erzeugte Code erzeugte überflüssig die 0 in Register r1.
Der Inhalt des Status Register (SREG) war in r0 gesichert, die push und pop Operationen sind überflüssig.

Die Funktion \_ctors_end wird vom Compiler eingebunden.
Die Funktion initialisiert die Umgebung des C Programms bevor die programmierte main Funktion aufgerufen wird.

```
00000068 <__ctors_end>:
  68: 11 24         eor r1, r1      : r1 = 0, das Null-Register
  6a: 1f be         out 0x3f, r1    ; Status Register SREG = 0
  6c: cf ef         ldi r28, 0xFF
  6e: d8 e0         ldi r29, 0x08
  70: de bf         out 0x3e, r29
  72: cd bf         out 0x3d, r28   ; SP (SPH:SPL) = 0x08ff, das Endes des RAM

00000074 <__do_copy_data>:
  74: 11 e0         ldi r17, 0x01
  76: a0 e0         ldi r26, 0x00
  78: b1 e0         ldi r27, 0x01   ; X = r27:26 = 0x100
  7a: e6 e1         ldi r30, 0x16
  7c: f1 e0         ldi r31, 0x01   ; Z = r31:30 = 0x116
  7e: 02 c0         rjmp  .+4       ; 0x84 <__do_copy_data+0x10>
  80: 05 90         lpm r0, Z+      ; load program memory (Z) with increment Z++
  82: 0d 92         st  X+, r0      ; store data (X) with increment X++
  84: a8 30         cpi r26, 0x08
  86: b1 07         cpc r27, r17    ; compare X = R27:26 == 0x0108
  88: d9 f7         brne  .-10      ; 0x80 <__do_copy_data+0xc>
  8a: 0e 94 67 00   call  0xce      ; 0xce <main>
```

Die Funktion \_ctors_end initialisert, wie immer, das Status-Register und den Stackpointer.

In diesem Programm werden noch die Daten initialisiert.

Über die 16-Bit Zeiger X = R27:R26 und Z = R31:R30 wird auf die Daten zugegriffen.
Die Daten werden aus dem Flash-Speicher gelesen `lpm` (load program memory)
und ins RAM geschrieben `st` (store).
Es werden 8 Bytes kopiert.
Die Laufbedingung für die Schleife ist X != 0x108.

Nach dem Kopieren der Daten wird die programmierte main Funktion aufgerufen.

Die Funktion beginnt mit dem Aufruf der programmierten initialise Funktion.

```
000000ac <initialisation>:
  ac: f8 94         cli             ; Interrupts abschalten bei der Initialisierung
  ae: 83 b7         in  r24, 0x33
  b0: 81 7f         andi r24, 0xF1
  b2: 83 bf         out 0x33, r24   ; im SMCR setzen SM0:2 = 0 = Sleep mode IDLE
  b4: 21 2c         mov r2, r1
  b6: 80 e2         ldi r24, 0x20
  b8: 84 b9         out 0x04, r24   ; DDRB = 0x20, B5 ist Ausgang
  ba: 8b e9         ldi r24, 0x9B
  bc: 87 bd         out 0x27, r24   ; OCR0A = 0x9B
  be: 82 e0         ldi r24, 0x02
  c0: 84 bd         out 0x24, r24   ; im TCCR0A setzen, WGM01:0 = 0x02
  c2: 95 e0         ldi r25, 0x05
  c4: 95 bd         out 0x25, r25   ; im TCCR0B setzen WGM02 = 0 und CS02:0 = 5
  c6: 80 93 6e 00   sts 0x006E, r24 ; im TIMSK0 setzen OCIE0A = 1
  ca: 78 94         sei             ; Interrupts einschalten
  cc: 08 95         ret
```

Der Idle sleep mode wird im SMCR (port 0x33, sleep mode control register) eingestellt.

Der Port B wird konfiguriert. B5 als Ausgang, alle anderen als Eingang.
(0x04, data direction register port B, DDRB).

Der Timer 0 wird programmiert:
- Vergleichswert A auf 0x9B = 156-1 (port 0x27, OCR0A, output compare register A timer 0).
- Timer löschen, wenn Match erreicht, CTC Mode WGM03:0 = 2
- Takt für den Timer: IO-Clock geteilt durch 1024, CS02:0 = 5
 (0x24 und 0x25, TCCR0A und TCCR0B, timer/counter control register timer 0 Teil A und B).
- Interrupt bei Match A auslösen (OCIE0A, output compare interrupt enable timer 0 match A).

Das Timer/Counter Control Register ist aufgeteilt in TCCR0A und RCCR0B.
Diese beiden Teile stehen nicht in Korrespondenz zu beiden Compare Register A und B.

Das Timer/Counter Interrupt Mask Register (TIMSK0) ist nur über das Mapping auf die
Speicheradresse 0x006E erreichbar.
Wird also über einen Store Befehl beschrieben.
Das TIMSK0 kann nicht mit `in` (in port) und `out` (out port) angesprichen werden.

Die Initialisierung wird in der main Funktion aufgerufen:

```
000000ce <main>:
  ce: 0e 94 56 00   call  0xac        ; 0xac <initialisation>
  d2: 80 e0         ldi   r24, 0x00   ; r24 = Index im Array der Wartezeiten
  d4: 20 e0         ldi   r18, 0x00   ; r18 = Zeit für das nächste Schalten
  d6: 30 e2         ldi   r19, 0x20
  d8: 42 2d         mov   r20, r2
  da: 93 b7         in    r25, 0x33
  dc: 91 60         ori   r25, 0x01
  de: 93 bf         out   0x33, r25   ; im SMCR Sleep erlauben, SE = 1
  e0: 88 95         sleep             : CPU schläft bis Interrupt
  e2: 93 b7         in    r25, 0x33
  e4: 9e 7f         andi  r25, 0xFE
  e6: 93 bf         out   0x33, r25   ; im SMCR Sleep verbieten, SE = 0
  e8: 42 17         cp    r20, r18    ; Vergleich aktuelle Zeit mit Wartezeit
  ea: b8 f3         brcs  .-18        ; 0xda <main+0xc>, weiter schlafen
  ec: 93 b1         in    r25, 0x03   ; r25 = PINB
  ee: 93 27         eor   r25, r19    ; Bit B5 invertieren
  f0: 95 b9         out   0x05, r25   ; PORTB = PINB mit invertiertem B5
  f2: 8f 5f         subi  r24, 0xFF   ; r24++, Index zur nächsten Wartezeit
  f4: e8 2f         mov   r30, r24
  f6: f0 e0         ldi   r31, 0x00
  f8: e0 50         subi  r30, 0x00
  fa: ff 4f         sbci  r31, 0xFF   ; Z = r31:r30 = Adresse im Array
  fc: 90 81         ld    r25, Z      ; Wartezeit laden
  fe: 91 11         cpse  r25, r1     ; Vergleich mit 0
 100: 01 c0         rjmp  .+2         ; 0x104 <main+0x36>
 102: 80 e0         ldi   r24, 0x00   ; am Ende des Arrays wieder auf Index 0
 104: e8 2f         mov   r30, r24
 106: f0 e0         ldi   r31, 0x00
 108: e0 50         subi  r30, 0x00
 10a: ff 4f         sbci  r31, 0xFF   ; Z = r31:r30 = Adresse im Array
 10c: 20 81         ld    r18, Z      ; Wartezeit bis zum nächsten Schalten
 10e: 21 2c         mov   r2, r1      ; 10ms Zähler auf 0
 110: e3 cf         rjmp  .-58        ; 0xd8 <main+0xa>
```

Alle lokalen Variablen der Funktion main sind vom Compiler in den
Registern r24, r18 abgelegt.
Der Stack wird für die lokalen Variablen nicht benutzt.

Die Wartezeiten sind im Array im SRAM gespeichert.
Für den Zugriff wird der 16-Bit Pointer Z = r31:r30 verwendet.

Der `sleep` Befehl legt die CPU schlafen.
Der Sleep-Modus ist auf den Idle Modus eingestellt, daher läuft der Timer 0 weiter.
Erreicht der Zähler TCNT0 den Wert OCR0A, wird ein Interupt ausgelöst.
Die ISR wird aufgerufen und inkrementiert den 10ms Zähler.
Danach wird der `sleep` Befehl beendet und die main Funktion wird weiter abgearbeitet.

Abhängig von der vergangenen Zeit wird die LED ein/ausgeschaltet
oder die CPU geht wieder über den `sleep` Befehl, bis die nächsten 10ms abgelaufen sind.

Über die verwendete Funktion `sleep_mode` der AVR-Libc wird das
Sleep Enable (SE) Bit im SMCR (Port 0x33, Sleep mode control register) nur
für den `sleep` Befehl gesetzt und damit der Schlaf-Modus aufrufbar.

Für das Ein/Ausschalten der LED an B5 wird der aktuelle Wert gelesen vom
PINB (Port 0x03, Port B input pins) und mit invertiertem Bit B5 geschrieben in
PRTB (Port 0x05, Port B data register).

Die Blink-Sequenz wird in einer Endlosschleife wiederholt.

## Quellen

- [RN-Wissen: avr-gcc](https://rn-wissen.de/wiki/index.php/Avr-gcc)

- [AVR Libc Home Page](https://www.nongnu.org/avr-libc/user-manual/index.html)

- [mikrocontroller.de: AVR-GCC-Tutorial](https://www.mikrocontroller.net/articles/AVR-GCC-Tutorial)

- [mikrocontroller.de: AVR-GCC](https://www.mikrocontroller.net/articles/AVR-GCC)

- [gsc-elektronic.net: LED blinkt mit dem Timer](http://www.gsc-elektronic.net/mikroelektronik/4_Led_Timer/4_Led_Timer.html)

- [ATmega 328P Datasheet](https://ww1.microchip.com/downloads/en/DeviceDoc/Atmel-7810-Automotive-Microcontrollers-ATmega328P_Datasheet.pdf)

- [AVR Instruction Set Manual](http://ww1.microchip.com/downloads/en/devicedoc/atmel-0856-avr-instruction-set-manual.pdf)
