package main

import (
	"fmt"
	"go.bug.st/serial"
	"time"
)

// Device name of the serial port created by the Arduino UNO board
const device = "/dev/ttyACM0"

// Mode of the serial connection to the Arduino UNO bootloader (optiload)
var mode = serial.Mode{
	BaudRate: 115_200,
	Parity:   serial.NoParity,
	DataBits: 8,
	StopBits: serial.OneStopBit,
}

// Constants defined in STK500 protocol
// and implemented in the bootloader.
const Resp_STK_OK byte = 0x10
const Resp_STK_FAILED byte = 0x11
const Resp_STK_UNKNOWN byte = 0x12
const Resp_STK_NODEVICE byte = 0x13
const Resp_STK_INSYNC byte = 0x14
const Resp_STK_NOSYNC byte = 0x15
const Sync_CRC_EOP byte = 0x20
const Cmnd_STK_GET_SYNC byte = 0x30
const Cmnd_STK_GET_SIGN_ON byte = 0x31
const Cmnd_STK_READ_FLASH byte = 0x70
const Cmnd_STK_READ_DATA byte = 0x71
const Cmnd_STK_READ_FUSE byte = 0x72
const Cmnd_STK_READ_LOCK byte = 0x73
const Cmnd_STK_READ_PAGE byte = 0x74
const Cmnd_STK_READ_SIGN byte = 0x75
const Cmnd_STK_READ_OSCCAL byte = 0x76
const Cmnd_STK_READ_FUSE_EXT byte = 0x77
const Cmnd_STK_READ_OSCCAL_EXT byte = 0x78

// Pause time to allow the Controller to init the bootloader.
// Up to 1000ms is possible.
// Optiboot waits up to 1s before jumping into the main program.
const waitController = 150 * time.Millisecond

// Maximal number of get sync attempts.
const maxGetSync = 20

func main() {
	fmt.Println("Connect Arduino via", device)
	// Create serial connection
	con, err := serial.Open(device, &mode)
	if err != nil {
		fmt.Println("can't open serial connection via " + device)
		panic(err)
	}
	// Timeout is essential.
	// If the controller does not answer, then the command is repeated.
	con.SetReadTimeout(waitController)
	defer con.Close()
	// Never more than 10 byte answers expected.
	// Attention: the serial modul does not append to the array!
	// The array length gives the maximal number of bytes to read.
	recv := make([]byte, 10)
	// Create connection -> set DTS -> reset -> start the bootloader
	// Sync communication
	time.Sleep(waitController)
	isSync := false
	for repeats := maxGetSync; !isSync && repeats > 0; repeats-- {
		// See: https://github.com/twischer/ESPWifiBootloader#flashing-an-attached-avrarduino
		// STK_GET_SYNC+CRC_EOP replaced by CRC_EOP+CRC_EOP
		con.Write([]byte{Sync_CRC_EOP})
		fmt.Print(".")
		n, err := con.Read(recv)
		if err != nil {
			fmt.Println("Read error:", err)
		}
		if n == 2 && recv[0] == Resp_STK_INSYNC && recv[1] == Resp_STK_OK {
			isSync = true
		}
	}
	if isSync {
		fmt.Println("Synchronised with bootloader")
		// Get controller signature
		con.Write([]byte{Cmnd_STK_READ_SIGN, Sync_CRC_EOP})
		n, err := con.Read(recv)
		if err != nil {
			fmt.Println("Read error:", err)
		}
		if recv[0] != Resp_STK_INSYNC || recv[n-1] != Resp_STK_OK {
			fmt.Println("unexpected answer: ", recv)
		}
		fmt.Println("Signature: ", recv[1:n-1])
	}
	fmt.Println("end.")
}
