/**
 * Project: AVR ATtiny USB Tutorial 
 * Author: yousef zamzam
 * Base on V-USB library
 * License: GNU GPL v3 (see License.txt)
 */
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>

#include "usbdrv.h"

#define F_CPU 12000000L
#include <util/delay.h>

#define USB_LED_OFF 0
#define USB_LED_ON  1
#define USB_DATA_OUT 2
#define USB_DATA_WRITE 3
#define USB_DATA_IN 4

static uchar replyBuf[16] = "Hello, USB!";
static uchar dataReceived = 0, dataLength = 0; // for USB_DATA_IN

/* PROGMEM char usbHidReportDescriptor[52] = { /* USB report descriptor, size must match usbconfig.h 
    0x05, 0x01,                    // USAGE_PAGE (Generic Desktop)
    0x09, 0x02,                    // USAGE (Mouse)
    0xa1, 0x01,                    // COLLECTION (Application)
    0x09, 0x01,                    //   USAGE (Pointer)
    0xA1, 0x00,                    //   COLLECTION (Physical)
    0x05, 0x09,                    //     USAGE_PAGE (Button)
    0x19, 0x01,                    //     USAGE_MINIMUM
    0x29, 0x03,                    //     USAGE_MAXIMUM
    0x15, 0x00,                    //     LOGICAL_MINIMUM (0)
    0x25, 0x01,                    //     LOGICAL_MAXIMUM (1)
    0x95, 0x03,                    //     REPORT_COUNT (3)
    0x75, 0x01,                    //     REPORT_SIZE (1)
    0x81, 0x02,                    //     INPUT (Data,Var,Abs)
    0x95, 0x01,                    //     REPORT_COUNT (1)
    0x75, 0x05,                    //     REPORT_SIZE (5)
    0x81, 0x03,                    //     INPUT (Const,Var,Abs)
    0x05, 0x01,                    //     USAGE_PAGE (Generic Desktop)
    0x09, 0x30,                    //     USAGE (X)
    0x09, 0x31,                    //     USAGE (Y)
    0x09, 0x38,                    //     USAGE (Wheel)
    0x15, 0x81,                    //     LOGICAL_MINIMUM (-127)
    0x25, 0x7F,                    //     LOGICAL_MAXIMUM (127)
    0x75, 0x08,                    //     REPORT_SIZE (8)
    0x95, 0x03,                    //     REPORT_COUNT (3)
    0x81, 0x06,                    //     INPUT (Data,Var,Rel)
    0xC0,                          //   END_COLLECTION
    0xC0,                          // END COLLECTION
};
*/
typedef struct{
    uchar   buttonMask;
    char    dx;
    char    dy;
    char    dWheel;
} report_t;

static report_t reportBuffer;
static uchar    idleRate;		

//this function will be called automatically when usbInit() is called
usbMsgLen_t usbFunctionSetup(uchar data[8]) {
    usbRequest_t *rq = (void *)data;

    if((rq->bmRequestType & USBRQ_TYPE_MASK) == USBRQ_TYPE_CLASS) {
        if(rq->bRequest == USBRQ_HID_GET_REPORT) {  
            usbMsgPtr = (void *)&reportBuffer; // we only have this one
            return sizeof(reportBuffer);
        } else if(rq->bRequest == USBRQ_HID_GET_IDLE) {
            usbMsgPtr = &idleRate;
            return 1;
        } else if(rq->bRequest == USBRQ_HID_SET_IDLE) {
            idleRate = rq->wValue.bytes[1];
        }
    }
	// will be active when control endpoint is used
	switch(rq->bRequest) { // custom command is in the bRequest field
	case USB_LED_ON:
	PORTB |= 0b10000000; // turn LED on
	return 0;
	case USB_LED_OFF:
	PORTB &= ~0b10000000; // turn LED off
	return 0;
	case USB_DATA_OUT: // send data to PC
	usbMsgPtr = replyBuf;
	return sizeof(replyBuf);
	case USB_DATA_WRITE: // modify reply buffer
	replyBuf[7] = rq->wValue.bytes[0];
	replyBuf[8] = rq->wValue.bytes[1];
	replyBuf[9] = rq->wIndex.bytes[0];
	replyBuf[10] = rq->wIndex.bytes[1];
	return 0;
	case USB_DATA_IN: // receive data from PC
	dataLength  = (uchar)rq->wLength.word;
	dataReceived = 0;
	if(dataLength  > sizeof(replyBuf)) // limit to buffer size
	dataLength  = sizeof(replyBuf);
	return USB_NO_MSG; // usbFunctionWrite will be called now
}
    return 0; // by default don't return any data
} // end of usbFunctionSetup

// This gets called when data is sent from PC to the device
USB_PUBLIC uchar usbFunctionWrite(uchar *data, uchar len) {
	uchar i;
	for(i = 0; dataReceived < dataLength && i < len; i++, dataReceived++)
	replyBuf[dataReceived] = data[i];
	return (dataReceived == dataLength); // 1 if we received it all, 0 if not
}

// main program
int main() {
	uchar i;
	DDRB =0b11111000; // Data Direction of portB
	PORTB=0b00001000; //turn led on just for tell that usb is connected
    wdt_enable(WDTO_1S); // enable 1s watchdog timer
    usbInit();
    usbDeviceDisconnect(); // enforce re-enumeration
    for(i = 0; i<250; i++) { // wait 500 ms
        wdt_reset(); // keep the watchdog happy
        _delay_ms(2);
    }
    usbDeviceConnect();
	sei(); // Enable interrupts after re-enumeration
	
	while(1) {
        wdt_reset(); // keep the watchdog happy
        usbPoll();
			// interrupt is send to endpoint 1
			if(usbInterruptIsReady()) {
			// print Ascll of a & b
			reportBuffer.dx = 'a';
			reportBuffer.dy = 'b';
			usbSetInterrupt((void *)&reportBuffer, sizeof(reportBuffer));
			}
			// toggle the switch to send bulk to endpoint3
			if ((PINB&(1<<1))== (1<<1)){
				PORTB |= 0b01000000;
				// print Ascll of x & y
				reportBuffer.dx = 'x';
				reportBuffer.dy = 'y';
				usbSetInterrupt3((void *)&reportBuffer, sizeof(reportBuffer));
				_delay_ms(1000);
				PORTB &= ~(0b01000000);
			}
		}   
    return 0;
}
