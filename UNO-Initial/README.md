# Initial state of the Arduino UNO R3

Der Arduino UNO verwendet 2 µController.
- ATmega 328P
- ATmega 16U2 für die Verbindung USB-Port ⇿ Serielle Schnittstelle ATmega 328P

Der Inhalt des Flashspeicher und der Zustand der Fuses beider µController
im Auslieferungszustand sind hier gespeichert.
Auf dem Arduino läuft das "Blink" Programm zum Test.

Beide µController sind verbunden mit 6poligen SPI Steckern auf der Platine.
(Nicht jeder Hersteller bringt Stiftleisten bei beiden Controller auf die Platine.)

Die Daten wurden gelesen mit USBasp ICSP-Programmierer und arvdude Programm.
Die Übertragung ist auf nur 100kHz eingestellt zur Sicherheit.


## dialout Permission

Unter Linux kann ein Benutzer in der Gruppe dialout die seriellen Schnittstellen nutzen.

```bash
su
usermod --append --groups dialout ulrich
```


## ATmega 8U2

```bash
avrdude -n -v -c usbasp -p m16u2 -B 100kHz 2> initial-m16u2-info.txt
avrdude -n -v -c usbasp -p m16u2 -B 100kHz -U flash:r:initial-m16u2.hex:i
```


## ATmega 328P

```bash
avrdude -n -v -c usbasp -p m328p -B 100kHz 2> initial-m328p-info.txt
avrdude -n -v -c usbasp -p m328p -B 100kHz -U flash:r:initial-m328p.hex:i
```

Der Flash Speicher des 328P kann auch über die USB Schnittstelle des Arduino ausgelesen werden.

Die Fuses können darüber nicht ausgelesen werden.
Für alle Fuses (lfuse, hfuse, efuse) wird der Wert "00" ausgegeben.

Der Arduino USB hat den Schnittstellennamen /dev/ttyACM0 vom System bekommen.


```bash
avrdude -n -v -c arduino -P /dev/ttyACM0 -p m328p
avrdude -n -v -c arduino -P /dev/ttyACM0 -p m328p -U flash:r:initial-m328p.hex.B:i
```

Durch die LEDs auf dem Arduino Board ist sichtbar, dass Daten über die
serielle Schnittstelle (RX, TX) laufen.


## Verbindung über den 16U2 prüfen

Der Controller 16U2 überträgt die Daten auf die serielle Schnittstelle des 328P.
Diese serielle Schnittstelle ist auf die Steckerleiste des Arduino geführt.

Zusätzlich wird über das DTR-Signal (beim Öffnen der seriellen Verbindung über USB)
ein Reset des 386P Controllers ausgelöst, damit der Bootloader startet.

Eine Brücke zwischen Rx und Tx gesteckt, dann kommen die Daten zurück.
Dabei kann der 328P Controller schlafen gelegt werden durch neg(Reset) auf GND.

Die Baudrate und weitere Parameter der seriellen Verbindung werden auf dem
Controller 16U2 verarbeitet.
Daher arbeitet das Echo bei verschiedenen Einstellungen.

Kontrolle der Verbindung mit dem tio (a simple TTY terminal I/O application) Programm.


```bash
tio -b 9600 /dev/ttyACM0
```

Der Arduino UNO kann so als USB - serial Umsetzer (5V Pegel) verwendet werden.
(Ohne den 328P Controller.)


## Datenübertragung von avrdude anzeigen

Die Ausgabe von avrdude kann so hoch eingestellt werden,
dass die einzelnen gesendeten und empfangenen Bytes angezeigt werden.


```bash
avrdude -n -vvvv -c arduino -P /dev/ttyACM0 -p m328p
```


## Kommunikation mit Bootloader

Den Verbindungsaufbau in Go programmieren und die Signature des Controllers auslesen:
read_signature.go

Die Parameter für die serielle Schnittstelle bei Arduino mit optiboot:
115200 baud, no parity, 8 data bits, 1 stop bit.

Der Bootloader startet nach dem Öffnen der seriellen Schnittstelle
über das DTR Signal vom 16U2 an den Reset Eingang des 328P.
Mit einen Elko wird das Reset Signal zeitlich geformt.

Das Timing ist kritisch.
Die Parameter habe aus der Beschreibung des ESPWifiBootloader entnommen.

An den TX und RX LEDs auf dem Board ist direkt sichtbar,
wenn die Kommunikation über die serielle Schnittstelle des 328P läuft.


## Quellen


- [Getting Started with Arduino UNO](https://www.arduino.cc/en/Guide/ArduinoUno)

- [Arduino UNO schematic](https://www.arduino.cc/en/uploads/Main/arduino-uno-schematic.pdf)

- [ATmega 328P Datasheet](https://ww1.microchip.com/downloads/en/DeviceDoc/Atmel-7810-Automotive-Microcontrollers-ATmega328P_Datasheet.pdf)

- [USBasp von Thomas Fischl](https://www.fischl.de/usbasp/)

- [How Arduino bootloader works](https://liudr.wordpress.com/2016/02/26/how-arduino-bootloader-works/)

- [AVR Bootloader in C](https://www.mikrocontroller.net/articles/AVR_Bootloader_in_C_-_eine_einfache_Anleitung)

- [Optiboot Bootloader for Arduino](https://github.com/Optiboot/optiboot)

- [STK500 Communication Protocol](http://ww1.microchip.com/downloads/en/AppNotes/doc2525.pdf)

- [twischer/ESPWifiBootloader/flashing AVR](https://github.com/twischer/ESPWifiBootloader#flashing-an-attached-avrarduino)
