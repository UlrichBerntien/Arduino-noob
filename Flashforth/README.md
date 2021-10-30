# Flashforth

FORTH ist eine Programmiersprache und auch ein komplettes System.
FORTH wurde entwickelt um direkt auf der Hardware zu laufen ohn ein Betriebsystem.
Daher eignet sich FORTH Systeme für Mikrocontroller.

Ein Vorteil bei der Entwicklung mit FORTH ist,
dass interaktiv auf dem Mikrocontroller ausprobiert werden kann.


## Umgebung

- Xubuntu 20.04 auf dem PC mit dem Paket avrdude.
- Arduino UNO R3 Board mit ATmega 328P.
- USBasp ICSP-Programmierer.

## Installieren

Laden des Flashforth Systems für den Arduino UNO.
Auf Github gibt es eine vorbereitete Intel-Hex-Datei für die ATmega 328.
Diese Datei kann auf dem Arduino UNO verwendet werden.

```bash
curl https://raw.githubusercontent.com/oh2aun/flashforth/master/avr/hex/328-16MHz-38400.hex -o 328-16MHz-38400.hex
```

Übertragen des Programms auf den Arduino UNO.
Die Programmierung kann nicht über den Bootloader des Arduino erfolgen.

Bei der Programmierung muss ein ICSP Programmiergerät verwendet werden.
Der Bootloader (Optiload) wird überschrieben.
Die Fuses werden neu programmiert.
Die Einstellungen sind auf der (Webseite)[] beschrieben.

_(
Versucht habe ich auch das Programmieren über den Optiboot Bootloader des Arduino.
Eine Fehlermeldung gab avrdude aus bei der Prüfung der geschriebenen Daten.
Es konnte nicht der gesamte 32KB Speichers der MCU beschrieben werden,
weil der Speicherbereich des Bootloaders nicht zugänglich ist.
Dennoch startet das Forth System.
Die Wortliste mit `words` kann noch ausgelesen werden.
Doch bereits ein `1 2 + .` bringt das Forth System zum Absturz.
Das flashforth muss über ein ICSP Programmiergerät auf den Controller geschrieben werden.
)_

```bash
avrdude -v -e -u -c usbasp -p m328p -B 100kHz -U flash:w:328-16MHz-38400.hex -U efuse:w:0xff:m -U hfuse:w:0xdf:m -U lfuse:w:0xff:m
```

Nach dem Flashen des FORTH Systems kann interaktiv mit dem Arduino gearbeitet werden.

Die serielle Schnittstelle des Arduino über den USART wird verwendet.
Diese Schnittstelle ist auf dem Arduino Boards mit dem USB-Seriell Konverter verbunden.
Der Arduino kann mit dem USB-Kabel angeschlossen werden.
Das FORTH System kann über die serielle Schnittstelle angesprochen werden.

Kommunikation mit dem Arduino über eine Terminal-Emulation,
tio - a simple TTY terminal I/O application.

```bash
tio --baudrate 38400 /dev/ttyACM0
```

Parameter für tio:
- Baudrate 38400 passend zur Konfiguration vom flashforth
- Das Gerät ist der USB-Seriell-Konverter auf dem Arduino
- Standardwerte: 8 Datenbits, 1 Stopbit, no parity

Beim Start des Terminals wird die serielle Schnittstelle eingeschaltet.
Das DTR Signal von der seriellen Schnittstelle führt zum Reset des Mikrocontrollers.

Das Forth System meldet sich nach dem Reset mit:

```
FlashForth 5 ATmega328P 04.09.2021
```


## Zurück auf Arduino Bootloader

Mit dem ICSP Programmierer kann der Bootloader wieder installiert werden.
Auch die Fuses müssen umprogrammiert werden.

Den Speicherinhalt mit Blink-Testprogramm und Optiboot habe ist in `initial-m328p.hex`.
Die Fuses für den Arduino müssen programmier werden auf:
lfuse = 0xFF, hfuse = 0xDE, efuse = 0xFD.

```bash
avrdude -v -e -u -c usbasp -p m328p -B 100kHz -U flash:w:../UNO-Initial/initial-m328p.hex -U efuse:w:0xFD:m -U hfuse:w:0xDE:m -U lfuse:w:0xFF:m
```

Danach kann der Arduino über den Optiboot Bootloader programmiert werden.

## Referenzen

- [Flashforth](https://flashforth.com/)

- [Installing on Atmega and Arduino](https://flashforth.com/atmega.html)

- [Flashforth auf Sourceforge](http://flashforth.sourceforge.net)

- [Flashforth auf Github](https://github.com/oh2aun/flashforth)
