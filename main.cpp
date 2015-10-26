//linuxcnc-ramps-passsthru.ino

// monitor pins from parallel port for change and toggle appropriate conencted pins on ramps/azteeg pro

#include "mbed.h"
#include "us_ticker_api.h"
#include "DebounceIn.h"

#include <algorithm>

#define LED1 P1_18
#define LED2 P1_19
#define LED3 P1_20
#define LED4 P1_21
#define LED5 P4_28
#define BUTTON1 P2_12

// Azteeg X5 mini (proto) pin mappings
#define X_STEP P2_1
#define X_DIR P0_11
#define X_ENABLE P0_10
#define X_MIN_ENDSTOP P1_24

#define Y_STEP P2_2
#define Y_DIR  P0_20
#define Y_ENABLE P0_19
#define Y_MIN_ENDSTOP P1_26

#define Z_STEP P2_3
#define Z_DIR P0_22
#define Z_ENABLE P0_21
#define Z_MIN_ENDSTOP P1_28

#define PROBE P1_29

#define E_STEP P2_0
#define E_DIR  P0_5
#define E_ENABLE P0_4

#define MOSFET_1 P2_7

// LinuxCNC LPC port definitions

// Mapping of parallel port pins to azteegx5 pins
// interrupts only on P0.* and P2.*
#define PP2  P0_16  // xpulse
#define PP3  P0_18  // Xdir
#define PP4  P0_26  // ypulse
#define PP5  P0_25  // Ydir
#define PP6  P2_11  // zpulse
#define PP7  P0_17  // Zdir
#define PP8  P2_6   // apulse
#define PP9  P2_8   // Adir
#define PP14 P0_15  // enable

//#define PP10 xx // estop
// NOTE on an intelligent breakout board this is handled direct by the breakout board and not through this
// however these can be wired direct to the parallel port.
#define PP11 P1_30 // probe optional
#define PP12 P3_26 // xlimit
#define PP13 P3_25 // ylimit
#define PP15 P1_23 // zlimit
//#define PP16 XX    // Bpulsej
#define PP17 P4_29 // spindle/relay

// LinuxCNC parallel port configuration - what pins the parallel port pins are conencted to
#define LCNC_X_STEP PP2
#define LCNC_X_DIR  PP3
#define LCNC_Y_STEP PP4
#define LCNC_Y_DIR  PP5
#define LCNC_Z_STEP PP6
#define LCNC_Z_DIR  PP7
//#define LCNC_A_STEP PP8 // uncomment for a 4th axis
//#define LCNC_A_DIR  PP9 / uncomment for a 4th axis

#define LCNC_ENABLE     PP14 // uncomment for enable pin

#define LCNC_SPINDLE    PP17 // use mosfet1

//#define LCNC_PROBE      PP11 // uncomment to use a probe on Z_MAX_LIMIT
#define LCNC_LIMIT_X    PP12
#define LCNC_LIMIT_Y    PP13
#define LCNC_LIMIT_Z    PP15

// this define will use interrupts to detect changes on the dir, otherwise it will be polled
// the downside of interupts is a noise/spurious interrupt will switch the dir until the next real one which is bad
// the upside is less latency on the dir pin
// recommended to leave this comment dout unless you have noiseless parallel port cables

//#define DIR_INTERRUPT


// global variables
volatile bool change = false;
uint8_t last_enable=1;
bool last_button= false;
bool led = false;
bool motors_enabled= false;
const int debounce_us= 10000;

// control pins
DigitalOut xStep(X_STEP);
DigitalOut xDir(X_DIR);
DigitalOut xEnable(X_ENABLE);
DigitalOut yStep(Y_STEP);
DigitalOut yDir(Y_DIR);
DigitalOut yEnable(Y_ENABLE);
DigitalOut zStep(Z_STEP);
DigitalOut zDir(Z_DIR);
DigitalOut zEnable(Z_ENABLE);
DigitalOut eStep(E_STEP);
DigitalOut eDir(E_DIR);
DigitalOut eEnable(E_ENABLE);

DigitalOut mosfet1(MOSFET_1);

DigitalOut led1(LED1, 0);
DigitalOut led2(LED2, 0);
DigitalOut led3(LED3, 0);
DigitalOut led4(LED4, 0);
DigitalOut led5(LED5, 1);
DebounceIn button1(BUTTON1, debounce_us);

DebounceIn xMinEndstop(X_MIN_ENDSTOP, debounce_us);
DebounceIn yMinEndstop(Y_MIN_ENDSTOP, debounce_us);
DebounceIn zMinEndstop(Z_MIN_ENDSTOP, debounce_us);
DebounceIn probe(PROBE, debounce_us);

// setup the parallel port monitor pins
DigitalOut lcncLimitX(LCNC_LIMIT_X);
DigitalOut lcncLimitY(LCNC_LIMIT_Y);
DigitalOut lcncLimitZ(LCNC_LIMIT_Z);

#ifdef DIR_INTERRUPT
InterruptIn  lcncXDir(LCNC_X_DIR);
InterruptIn  lcncYDir(LCNC_Y_DIR);
InterruptIn  lcncZDir(LCNC_Z_DIR);
#ifdef LCNC_A_STEP
InterruptIn  lcncADir(LCNC_A_DIR);
#endif
#else
DigitalIn  lcncXDir(LCNC_X_DIR);
DigitalIn  lcncYDir(LCNC_Y_DIR);
DigitalIn  lcncZDir(LCNC_Z_DIR);
#ifdef LCNC_A_STEP
DigitalIn  lcncADir(LCNC_A_DIR);
#endif
#endif

InterruptIn  lcncXStep(LCNC_X_STEP);
InterruptIn  lcncYStep(LCNC_Y_STEP);
InterruptIn  lcncZStep(LCNC_Z_STEP);
#ifdef LCNC_A_STEP
InterruptIn  lcncAStep(LCNC_A_STEP);
#endif
#ifdef LCNC_ENABLE
DigitalIn  lcncEnable(LCNC_ENABLE);
#endif

#ifdef LCNC_PROBE
DigitalOut lcncProbe(LCNC_PROBE);
#endif

#ifdef LCNC_SPINDLE
DigitalIn lcncSpindle(LCNC_SPINDLE);
#endif

// Interrupt handlers for pinchanges
// just mirror the change
// specs: 2.5us latency, 100Khz max
void lcncXStepRise()
{
	xStep= 1;
}
void lcncXStepFall()
{
	xStep= 0;
}
void lcncYStepRise()
{
	yStep= 1;
}
void lcncYStepFall()
{
	yStep= 0;
}
void lcncZStepRise()
{
	zStep= 1;
}
void lcncZStepFall()
{
	zStep= 0;
}
#ifdef LCNC_A_STEP
void lcncAStepRise()
{
	aStep= 1;
}
void lcncAStepFall()
{
	aStep= 0;
}
#endif

#ifdef DIR_INTERRUPT
void lcncXDirRise()
{
	xDir= 1;
}
void lcncXDirFall()
{
	xDir= 0;
}
void lcncYDirRise()
{
	yDir= 1;
}
void lcncYDirFall()
{
	yDir= 0;
}
void lcncZDirRise()
{
	zDir= 1;
}
void lcncZDirFall()
{
	zDir= 0;
}

#ifdef LCNC_A_DIR
void lcncADirRise()
{
	aDir= 1;
}
void lcncADirFall()
{
	aDir= 0;
}
#endif

#endif

void enable_motors(bool on)
{
	uint8_t v = on ? 0 : 1;
	motors_enabled= on;

	// enable or disable drivers
	xEnable= v;
	yEnable= v;
	zEnable= v;
	eEnable= v;
	led5= v;
}

// setup the currents for the steppers
#include "I2C.h" // mbed.h lib
void i2c_send(mbed::I2C *i2c, char first, char second, char third ){
    i2c->start();
    i2c->write(first);
    i2c->write(second);
    i2c->write(third);
    i2c->stop();
}

#define MAX_CURRENT 2.4F // azteeg drv8825
//#define MAX_CURRENT 2.0F // smoothieboard

mbed::I2C i2c(p9, p10);
void setCurrents()
{
    i2c.frequency(20000);
    //static const float currents[]{1.2F, 1.2F, 1.2F, 1.2F}; // currents for X Y Z A
    static const float currents[]{0.7F, 0.7F, 0.7F, 0.7F}; // currents for X Y Z A
    static const char addresses[]{ 0x00, 0x10, 0x60, 0x70 };
    static const float factor= 152.1F; // azteeg X5 mini proto
    //static const float factor= 106.1F; // azteeg X5 mini production
    //static const float factor= 113.33F; // smoothieboard

    for (int i = 0; i < 4; ++i) {
	    float current = std::min( std::max( currents[i], 0.0F ), MAX_CURRENT );
    	char addr = 0x58;

        // Initial setup
        i2c_send(&i2c, addr, 0x40, 0xff );
        i2c_send(&i2c, addr, 0xA0, 0xff );

        // Set actual wiper value
        char wiper= ceilf(factor*current);
        i2c_send(&i2c, addr, addresses[i], wiper );
    }
    wait_ms(500);
}

void setup()
{
	// start with motors disabled
	enable_motors(false);

	// set digipot currents
	setCurrents();

	// set to pull none
	lcncXDir.mode(PullNone);
	lcncYDir.mode(PullNone);
	lcncZDir.mode(PullNone);
#ifdef LCNC_A_STEP
	lcncADir.mode(PullNone);
#endif
	lcncXStep.mode(PullNone);
	lcncYStep.mode(PullNone);
	lcncZStep.mode(PullNone);
#ifdef LCNC_A_STEP
	lcncAStep.mode(PullNone);
#endif
#ifdef LCNC_ENABLE
	lcncEnable.mode(PullNone);
#endif

#ifdef LCNC_PROBE
	lcncProbe.mode(PullNone);
#endif

#ifdef LCNC_SPINDLE
	lcncSpindle.mode(PullNone);
#endif

#ifdef LCNC_PROBE
	lcncProbe= probe;
#endif

#ifdef LCNC_SPINDLE
	mosfet1= lcncSpindle;
#endif

	// initialize mirror pins
	lcncLimitX= xMinEndstop;
	lcncLimitY= yMinEndstop;
	lcncLimitZ= zMinEndstop;
	led2= xMinEndstop;
	led3= yMinEndstop;
	led4= zMinEndstop;

	xStep= lcncXStep;
	xDir= lcncXDir;
	yStep= lcncYStep;
	yDir= lcncYDir;
	zStep= lcncZStep;
	zDir=  lcncZDir;
#if defined(LCNC_A_STEP) && defined(LCNC_A_DIR)
	eStep= lcncAStep;
	eDir=  lcncADir;
#endif

#ifndef LCNC_ENABLE
	// always enable motors
	enable_motors(true);
#endif

	// Setup interrupts on the LPC monitor pins for step pins
	lcncXStep.rise(lcncXStepRise); lcncXStep.fall(lcncXStepFall);
	lcncYStep.rise(lcncYStepRise); lcncYStep.fall(lcncYStepFall);
	lcncZStep.rise(lcncZStepRise); lcncZStep.fall(lcncZStepFall);
#ifdef LCNC_A_STEP
	lcncAStep.rise(lcncAStepRise); lcncAStep.fall(lcncAStepFall);
#endif

#ifdef DIR_INTERRUPT
	lcncXDir.rise(lcncXDirRise); lcncXDir.fall(lcncXDirFall);
	lcncYDir.rise(lcncYDirRise); lcncYDir.fall(lcncYDirFall);
	lcncZDir.rise(lcncZDirRise); lcncZDir.fall(lcncZDirFall);
#ifdef LCNC_A_DIR
	lcncADir.rise(lcncADirRise); lcncADir.fall(lcncADirFall);
#endif

#endif
}

void loop()
{
	uint32_t ledcnt= 0;
	while(1) {
		uint8_t v;

#ifndef DIR_INTERRUPT
		// monitor dir pins
		// latency 3.75us, max frequency: 16khz, minimum pulse width: 30us
		if((v=lcncXDir) != xDir) {
			xDir= v;
		}
		if((v=lcncYDir) != yDir) {
			yDir= v;
		}
		if((v=lcncZDir) != zDir) {
			zDir= v;
		}
#ifdef LCNC_A_DIR
		if((v=lcncADir) != aDir) {
			aDir= v;
		}
#endif

#endif

#ifdef LCNC_ENABLE
		// monitor enable pin
		if((v=lcncEnable) != last_enable) {
			last_enable = v;
			enable_motors(v==0);
		}
#endif
		// monitor limit switches, they are debounced
		if((v=xMinEndstop) != lcncLimitX) {
			lcncLimitX= v;
			led2= v;
		}
		if((v=yMinEndstop) != lcncLimitY) {
			lcncLimitY= v;
			led3= v;
		}
		if((v=zMinEndstop) != lcncLimitZ) {
			lcncLimitZ= v;
			led4= v;
		}

		#ifdef LCNC_PROBE
		if((v=probe) != lcncProbe) {
			lcncProbe= v;
		}
		#endif

		#ifdef LCNC_SPINDLE
		if((v=lcncSpindle) != mosfet1) {
			mosfet1= v;
		}
		#endif

		// handle play button toggling the enable, however this will be overriden by the lncEnable if used
		if(button1 == 0) {
			if(!last_button) {
				enable_motors(!motors_enabled);
				last_button= true;
			}
		}else if(last_button) {
			last_button= false;
		}

		if((ledcnt++ % 200000) == 0) {
			led1= !led1;
		}
	}
}

int main()
{
#if 0
	// test steppers
	//uint32_t s= us_ticker_read();
	setCurrents();
	xEnable= 0;
	while(1) {
		for (int i = 0; i < 800*100; ++i) {
		    xStep= 1;
		    wait_us(10);
		    xStep= 0;
		    wait_us(100); //
		}

		xDir= !xDir;
		wait_us(10);
		led1= !led1;
	}

#else
	setup();
	while(1) {
		loop();
	}
#endif
}
