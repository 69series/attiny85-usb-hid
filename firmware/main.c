/*
ATtiny85 USB HID Keyboard - Custom Firmware
Author: Narendra Sagolsem
PB3=USB D-, PB4=USB D+, PB2=LED
*/

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <string.h>
#include "usbdrv/usbdrv.h"

const PROGMEM char usbHidReportDescriptor[45] = {
    0x05,0x01,0x09,0x06,0xA1,0x01,0x05,0x07,
    0x19,0xE0,0x29,0xE7,0x15,0x00,0x25,0x01,
    0x75,0x01,0x95,0x08,0x81,0x02,0x95,0x01,
    0x75,0x08,0x81,0x03,0x95,0x06,0x75,0x08,
    0x15,0x00,0x25,0x65,0x05,0x07,0x19,0x00,
    0x29,0x65,0x81,0x00,0xC0
};

typedef struct {
    uint8_t modifier;
    uint8_t reserved;
    uint8_t keycode[6];
} KeyReport;

static KeyReport reportBuffer;
static uint8_t idleRate;

/* Modifier keys */
#define MOD_LCTRL   0x01
#define MOD_LSHIFT  0x02
#define MOD_LALT    0x04
#define MOD_LGUI    0x08

/* Keycodes */
#define KEY_ENTER   0x28
#define KEY_ESC     0x29
#define KEY_TAB     0x2B
#define KEY_SPACE   0x2C
#define KEY_F11     0x44

/* Global blink counter for task LED rhythm */
static uint16_t blinkCounter = 0;
static uint8_t taskRunning = 0;   /* 0=init phase, 1=task phase */

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

/* 16.5MHz crystal timing calibration */
void calibrateOscillator(void) {
    uchar step = 128;
    uchar trialValue = 0, optimumValue;
    int x, optimumDev,
        targetValue = (unsigned)(1499 * (double)F_CPU / 10.5e6 + 0.5);
    do {
        OSCCAL = trialValue + step;
        x = usbMeasureFrameLength();
        if(x < targetValue) trialValue += step;
        step >>= 1;
    } while(step > 0);
    optimumValue = trialValue;
    optimumDev = x;
    for(OSCCAL = trialValue - 1; OSCCAL <= trialValue + 1; OSCCAL++) {
        x = usbMeasureFrameLength() - targetValue;
        if(x < 0) x = -x;
        if(x < optimumDev) { optimumDev = x; optimumValue = OSCCAL; }
    }
    OSCCAL = optimumValue;
}


/* poll-safe delay - auto blinks LED every 50ms during task phase */
void pollDelay(uint16_t ms) {
    for(uint16_t i = 0; i < ms; i++) {
        usbPoll();
        _delay_ms(1);

        if(taskRunning) {
            blinkCounter++;
            if(blinkCounter >= 50) {  /* 50ms blink */
                PORTB ^= (1 << PB2);   /* toggle LED */
                blinkCounter = 0;
            }
        }
    }
}


/* blink used ONLY for init and done signals */
void blinkLED(uint8_t times, uint16_t ms) {
    taskRunning = 0;   /* pause auto-blink during manual blink */
    for(uint8_t i = 0; i < times; i++) {
        PORTB |=  (1 << PB2);
        pollDelay(ms);
        PORTB &= ~(1 << PB2);
        pollDelay(ms);
    }
}

void ledOff(void) { PORTB &= ~(1 << PB2); }

void sendNull(void) {
    while(!usbInterruptIsReady()) { usbPoll(); _delay_ms(5); }
    memset(&reportBuffer, 0, sizeof(reportBuffer));
    usbSetInterrupt((void *)&reportBuffer, sizeof(reportBuffer));
}

void sendKey(uint8_t modifier, uint8_t keycode) {
    while(!usbInterruptIsReady()) { usbPoll(); _delay_ms(5); }
    memset(&reportBuffer, 0, sizeof(reportBuffer));
    reportBuffer.modifier   = modifier;
    reportBuffer.keycode[0] = keycode;
    usbSetInterrupt((void *)&reportBuffer, sizeof(reportBuffer));

    while(!usbInterruptIsReady()) { usbPoll(); _delay_ms(5); }
    memset(&reportBuffer, 0, sizeof(reportBuffer));
    usbSetInterrupt((void *)&reportBuffer, sizeof(reportBuffer));
    _delay_ms(30);
}

void sendString(const char *str) {
    while(*str) {
        char c = *str++;
        uint8_t kc = 0, mod = 0;
        if     (c>='a'&&c<='z') { kc = 4+(c-'a'); }
        else if(c>='A'&&c<='Z') { kc = 4+(c-'A'); mod = MOD_LSHIFT; }
        else if(c>='1'&&c<='9') { kc = 30+(c-'1'); }
        else if(c=='0')         { kc = 39; }
        else if(c==' ')         { kc = KEY_SPACE; }
        else if(c=='\n')        { kc = KEY_ENTER; }
        else if        (c=='.') { kc = 55; }
        else if        (c=='/') { kc = 56; }
        else if        (c==':') { kc = 51; mod = MOD_LSHIFT; }
        else if (c == '#') { kc = 32; mod = MOD_LSHIFT; }
        if(kc) sendKey(mod, kc);
        _delay_ms(20);
    }
}

/* ─────────────────────────────────────────── */

int main(void) {
    DDRB  |=  (1 << PB2);
    PORTB &= ~(1 << PB2);

    /* ── USB INIT ── */
    cli();
    usbDeviceDisconnect();
    pollDelay(250);
    usbDeviceConnect();
    usbInit();
    calibrateOscillator();
    sei();

    /* first-char fix pass 1 */
    sendNull();

    /* 2x quick blink = ready signal (init phase, no auto-blink) */
    blinkLED(2, 50);
    pollDelay(100);

    /* first-char fix pass 2 */
    sendNull();

    /* ── START TASK PHASE - auto blink begins now ── */
    taskRunning = 1;
    blinkCounter = 0;

    /* AUTOMATION TASK ------------------- */
    /* Win+R → cmd → Enter */
    pollDelay(5);
    sendKey(MOD_LGUI, 0x15);
    pollDelay(100);
    sendString("cmd");
    pollDelay(100);
    sendKey(0, KEY_ENTER);
    pollDelay(400);

    sendString("FIRMWARE DESIGN BY NARENDRA SAGOLSEM");
    pollDelay(100);
    sendKey(0, KEY_ENTER);
    pollDelay(800);

    /* TASK DONE------------------- */

    /* stop auto-blink, clean LED state */
    taskRunning = 0;
    ledOff();

    /* 5x slow blink = done! */
    blinkLED(5, 900);
    ledOff();

    while(1) { usbPoll(); }
    return 0;
}