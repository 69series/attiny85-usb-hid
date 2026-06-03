/*
 * ATtiny85 USB HID Keyboard
 * Bare metal C - V-USB library
 * Author: Narendra Sagolsem
 *
 * PB3 = USB D+
 * PB4 = USB D-
 * PB2 = Status LED
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include "usbdrv/usbdrv.h"

/* HID Report Descriptor - Keyboard */
const PROGMEM char usbHidReportDescriptor[45] = {
    0x05, 0x01,  /* Usage Page (Generic Desktop) */
    0x09, 0x06,  /* Usage (Keyboard) */
    0xA1, 0x01,  /* Collection (Application) */
    0x05, 0x07,  /* Usage Page (Keyboard) */
    0x19, 0xE0,  /* Usage Minimum (224) */
    0x29, 0xE7,  /* Usage Maximum (231) */
    0x15, 0x00,  /* Logical Minimum (0) */
    0x25, 0x01,  /* Logical Maximum (1) */
    0x75, 0x01,  /* Report Size (1) */
    0x95, 0x08,  /* Report Count (8) */
    0x81, 0x02,  /* Input (Data, Variable, Absolute) */
    0x95, 0x01,  /* Report Count (1) */
    0x75, 0x08,  /* Report Size (8) */
    0x81, 0x03,  /* Input (Constant) */
    0x95, 0x06,  /* Report Count (6) */
    0x75, 0x08,  /* Report Size (8) */
    0x15, 0x00,  /* Logical Minimum (0) */
    0x25, 0x65,  /* Logical Maximum (101) */
    0x05, 0x07,  /* Usage Page (Keyboard) */
    0x19, 0x00,  /* Usage Minimum (0) */
    0x29, 0x65,  /* Usage Maximum (101) */
    0x81, 0x00,  /* Input (Data, Array) */
    0xC0         /* End Collection */
};

/* keyboard report structure */
typedef struct {
    uint8_t modifier;
    uint8_t reserved;
    uint8_t keycode[6];
} KeyReport;

static KeyReport reportBuffer;
static uint8_t idleRate;

/* USB setup handler */
usbMsgLen_t usbFunctionSetup(uint8_t data[8]) {
    usbRequest_t *rq = (usbRequest_t *)data;

    if((rq->bmRequestType & USBRQ_TYPE_MASK) == USBRQ_TYPE_CLASS) {
        if(rq->bRequest == USBRQ_HID_GET_REPORT) {
            usbMsgPtr = (void *)&reportBuffer;
            return sizeof(reportBuffer);
        } else if(rq->bRequest == USBRQ_HID_GET_IDLE) {
            usbMsgPtr = &idleRate;
            return 1;
        } else if(rq->bRequest == USBRQ_HID_SET_IDLE) {
            idleRate = rq->wValue.bytes[1];
        }
    }
    return 0;
}

/* send a keystroke */
void sendKey(uint8_t modifier, uint8_t keycode) {
    /* wait for USB ready */
    while(!usbInterruptIsReady()) {
        usbPoll();
    }
    /* press key */
    reportBuffer.modifier = modifier;
    reportBuffer.reserved = 0;
    reportBuffer.keycode[0] = keycode;
    usbSetInterrupt((void *)&reportBuffer, sizeof(reportBuffer));

    /* wait then release */
    while(!usbInterruptIsReady()) {
        usbPoll();
    }
    /* release key */
    reportBuffer.modifier = 0;
    reportBuffer.keycode[0] = 0;
    usbSetInterrupt((void *)&reportBuffer, sizeof(reportBuffer));

    _delay_ms(50);
}

/* send a string of keypresses */
void sendString(const char *str) {
    while(*str) {
        char c = *str++;
        uint8_t keycode = 0;
        uint8_t modifier = 0;

        if(c >= 'a' && c <= 'z') {
            keycode = 4 + (c - 'a');
        } else if(c >= 'A' && c <= 'Z') {
            keycode = 4 + (c - 'A');
            modifier = 0x02; /* left shift */
        } else if(c == ' ') {
            keycode = 0x2C;
        } else if(c == '\n') {
            keycode = 0x28; /* enter */
        }

        if(keycode) {
            sendKey(modifier, keycode);
        }
        _delay_ms(20);
    }
}

/* blink LED on PB2 */
void blinkLED(uint8_t times) {
    for(uint8_t i = 0; i < times; i++) {
        PORTB |= (1 << PB2);
        _delay_ms(100);
        PORTB &= ~(1 << PB2);
        _delay_ms(100);
    }
}

int main(void) {
    /* setup PB2 as output for LED */
    DDRB |= (1 << PB2);
    PORTB &= ~(1 << PB2);

    /* initialize USB */
    usbInit();
    usbDeviceDisconnect();
    _delay_ms(300);
    usbDeviceConnect();

    sei(); /* enable interrupts */

    /* blink 3 times = ready */
    blinkLED(3);

    uint8_t triggered = 0;

    while(1) {
        usbPoll();

        if(!triggered) {
            _delay_ms(2000); /* wait 2s after connect */

            /*
             * YOUR ACTION HERE:
             * Example 1 - type a message:
             * sendString("Hello from ATtiny85\n");
             *
             * Example 2 - open run dialog and clear temp:
             * sendKey(0x08, 0x15); // WIN + R
             * _delay_ms(500);
             * sendString("%temp%\n");
             * _delay_ms(1000);
             * sendKey(0x01, 0x04); // CTRL + A
             * _delay_ms(200);
             * sendKey(0x02, 0x4C); // SHIFT + DEL
             */

            sendString("Hello from ATtiny85\n");

            blinkLED(2); /* blink 2 times = done */
            triggered = 1;
        }
    }

    return 0;
}