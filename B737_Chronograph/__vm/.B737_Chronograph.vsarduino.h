/* 
	Editor: https://www.visualmicro.com/
			This file is for intellisense purpose only.
			Visual micro (and the arduino ide) ignore this code during compilation. This code is automatically maintained by visualmicro, manual changes to this file will be overwritten
			The contents of the _vm sub folder can be deleted prior to publishing a project
			All non-arduino files created by visual micro and all visual studio project or solution files can be freely deleted and are not required to compile a sketch (do not delete your own code!).
			Note: debugger breakpoints are stored in '.sln' or '.asln' files, knowledge of last uploaded breakpoints is stored in the upload.vmps.xml file. Both files are required to continue a previous debug session without needing to compile and upload again
	
	Hardware: Teensy++ 2.0, Platform=teensy, Package=teensy
*/

#if defined(_VMICRO_INTELLISENSE)

#ifndef _VSARDUINO_H_
#define _VSARDUINO_H_
#define __HARDWARE_at90usb1286__
#define __HARDWARE_AT90USB1286__
#define __HARDWARE_AT90usb1286__
#define TEENSYDUINO 148
#define ARDUINO_ARCH_AVR
#define ARDUINO 10809
#define F_CPU 16000000L
#define USB_SERIAL
#define LAYOUT_US_ENGLISH
#define __cplusplus 201103L
#define __AVR__
#define __extension__
#define  __attribute__(x)
typedef void *__builtin_va_list;

#define __extension__
#define __ATTR_PURE__
#define __ATTR_CONST__
#define __inline__
#define __asm__ 
#define __volatile__

#define NEW_H
// C:\TESTBED\arduino\ide_1-8-8\hardware\tools\arm\arm-none-eabi\include\sys\types.h

typedef signed char        int8_t;
typedef short              int16_t;
typedef int                int32_t;
typedef long long          int64_t;
typedef unsigned char      uint8_t;
typedef unsigned short     uint16_t;
typedef unsigned int       uint32_t;
typedef unsigned long long uint64_t;
typedef unsigned char boolean;
typedef unsigned char byte;
typedef unsigned int uint;

// Defines all pins from 
//C:\TESTBED\arduino\ide_1-8-8\hardware\teensy\avr\cores\teensy\pins_arduino.h
#define __AVR_ATmega32U4__

// C:\TESTBED\arduino\ide_1-8-8\hardware\teensy\avr\cores\teensy\wiring.h
#define sei()
#define cli()
#define interrupts() sei()
#define noInterrupts() cli()

// C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Tools\MSVC\14.21.27702\include\stdint.h
typedef unsigned char      uint8_t;


#include "arduino.h"
#undef cli
#define cli()
#include "B737_Chronograph.ino"
#endif
#endif
