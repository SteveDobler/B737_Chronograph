/*
Name:		VS_Chronograph.ino
Created:	Started 9/19/2018
Author:		Steve Dobler
Modified:	6/26/19
*/

// 1st Branch Attempt

/*******************************************************************************************************************\
|******************************************* B737 Chronograph Layout ***********************************************|
\*******************************************************************************************************************/

//B737 Chronograph Button Function Description: 
// You Tube https://www.youtube.com/watch?v=hhjirV54CP4
// You Tube https://www.youtube.com/watch?v=QUIBxDq5vrY

/* 737 ChronographDisplay & Button layout  [o] = Button

			   ----------------------------------------------
			  || CHR                              TIME/DATE ||
			  || [o]                                  [o]   ||
			  ||                                        SET ||
			  ||                    [60]                [o] ||
			  ||                                            ||
			  ||          [50]               [10]           ||
			  ||                                            ||
			  ||               [1][2]:[0][0]                ||  <-- TIME / DATE Display
			  ||                     o------------->        ||  <-- Sweep Hand
			  ||               [4][8]:[1][5]                ||  <-- Elapsed Time Display
			  ||                                            ||
			  ||          [40]               [20]           ||
			  ||                                            ||
			  ||                    [30]                    ||
			  || ET                                      +  ||
			  || [o]                                  - [o] ||
			  ||  [o] RESET                          [o]    ||
			   ----------------------------------------------
*/

/*******************************************************************************************************************\
|********************************************** Chronograph Libraries **********************************************|
\*******************************************************************************************************************/

#include <Bounce2.h>		// This library prevents false pushbuttons due to mechanical bounce
#include <Wire.h>			// Library for I2C serial communication to real time clock (DS3231)
#include <DS3231.h>			// Library for DS3231 real time clock
#include <TM1637Display.h>	// Library for the two TM1637 Display modules
#include <AccelStepper.h>	// Stepper Motor Library - Non-blocking execution

/*******************************************************************************************************************\
|************************************* Arduino Pushbutton Pin Definitions ******************************************|
\*******************************************************************************************************************/

const int btnET = 13;			// ET Button			(Arduino Pin 13) 
const int btnTimeDate = 9;		// TIME / DATE Button	(Arduino Pin  9) 
const int btnSet = 10;			// SET Button			(Arduino Pin 10) 
const int btnPlus = 7;			// + Button				(Arduino Pin  7) 
const int btnMinus = 11;		// - Button				(Arduino Pin 11) 
const int btnCHR = 8;			// CHR Button			(Arduino Pin  8) 
const int btnReset = 12;		// RESET Button			(Arduino Pin 12) 


/*******************************************************************************************************************\
|******************** Arduino Pin Definitions for Stepper Motor & Home Position Switch *****************************|
\*******************************************************************************************************************/

// Stepper Motor Module Driver Arduino Pins
#define motorPin1  6	// IN1 on the ULN2003 stepper motor driver board
#define motorPin2  5	// IN2 on the ULN2003 stepper motor driver board
#define motorPin3  4	// IN3 on the ULN2003 stepper motor driver board
#define motorPin4  3	// IN4 on the ULN2003 stepper motor driver board


/*******************************************************************************************************************\
|************************* Arduino Pin Definitions for the Stepper Motor Home Position Sensor **********************|
\*******************************************************************************************************************/

// Stepper Motor Home Position Hall Effect Sensor Arduino Pin
#define home_switch 2	// Pin 2 connected to Home Switch (Hall Effect Sensor)


/*******************************************************************************************************************\
|********************************* Create an Instance of Bounce for each Pushbutton ********************************|
\*******************************************************************************************************************/

// The <Bounce2.h> library takes care of debouncing the pushbuttons so you don't get multiple keypresses

int debounceTime = 10;  // 10 ms de-bounce time. This time can be adjusted if multiple pushes are detected

// See above diagram for the location of each of these pushbuttons

Bounce ETbutton = Bounce(btnET, debounceTime);
Bounce TimeDatebutton = Bounce(btnTimeDate, debounceTime);
Bounce Setbutton = Bounce(btnSet, debounceTime);
Bounce Plusbutton = Bounce(btnPlus, debounceTime);
Bounce Minusbutton = Bounce(btnMinus, debounceTime);
Bounce CHRbutton = Bounce(btnCHR, debounceTime);
Bounce Resetbutton = Bounce(btnReset, debounceTime);

/******************************************************************************************************************\
|************************************** Create Variables for Chonograph Timer *************************************|
\******************************************************************************************************************/

unsigned long chronoTime;		// milliseconds holder
unsigned long chronoTimeOld;	// old milliseconds holder
int sec = 0;					// second holder
int mins = 0;					// minutes holder
bool tick = true;				// colon flag for chronograph display
int secondsCounter = 0;			// Used to track the stepper motor ticks
//int oneSecondStepSize = 100;

/*******************************************************************************************************************\
|**************************************** Create Variables for Sweep Hand ******************************************|
\*******************************************************************************************************************/

#define HALFSTEP 8		// Stepper Motor Mode

// Initialize with pin sequence IN1-IN3-IN2-IN4 for using the AccelStepper with 28BYJ-48
AccelStepper chronoStepper(HALFSTEP, motorPin1, motorPin3, motorPin2, motorPin4);

long initial_homing = -1;   	// Stepper Travel Variable. Used to Home Stepper at startup
unsigned long position = 110;	// Varaible to store home position of the stepper motor

/*******************************************************************************************************************\
|************************* Arduino TM1637 4 Digit  (TIME / DATE Display) Pin Definitions ***************************|
\*******************************************************************************************************************/

// Set up for TM1637 4 Digit Display Module (Top Display Time / Date)
#define CLK_D_Clock 22			// Clock Pin
#define DIO_D_Clock 24			// Data Pin

// Set up for TM1637 4 Digit Display Module (Bottom Display - Chronograph Timer)
#define CLK_Chronograph 26     // Clock Pin
#define DIO_Chronograph 28     // Data Pin

/*******************************************************************************************************************\
|**************** Variables for timers used to have the display blink when setting time / Date *********************|
\*******************************************************************************************************************/

// stores the value of millis() in each iteration of loop()
unsigned long currentMillis = 0;

/*******************************************************************************************************************\
|************************************ Create an Instance of the DS3231 Clock ***************************************|
\*******************************************************************************************************************/

DS3231 clock;
#define   DS3231_I2C_ADDRESS 0x68  // Hex address of the DS3231 Clock Module

/*******************************************************************************************************************\
|***************************** Create an Instance of the TM1637 Display Modules ************************************|
\*******************************************************************************************************************/

TM1637Display tm1637(CLK_D_Clock, DIO_D_Clock);			// Display for TIME & DATE
TM1637Display display(CLK_Chronograph, DIO_Chronograph);	// Display for Chronograph Timer

/*******************************************************************************************************************\
|**************************************** Arduino DS3231 Pin Definitions *******************************************|
\*******************************************************************************************************************/

/*
The wire library uses Arduino SDA and SLC pins to communicate with the DS3231 Real time clock
For the Arduino Mega, the SDA signal is Pin #20 and SLC is Pin #21. There is no need to have a
"#define" statement for these two pins
*/

/*******************************************************************************************************************\
|******************************************** Variables for Pushbutton Presses *************************************|
\*******************************************************************************************************************/

// These variables hold the number of times each pushbutton is pressed.  The count is used by
// the state machine to determine what action shoudl be taken for each count.

int ETButtonCount;
int TimeDateButtonCount;
int SetButtonCount;
int PlusButtonCount;
int MinusButtonCount;
int ResetButtonCount;
int CHRButtonCount;

/*******************************************************************************************************************\
|**************************************** Variables for Time and Date Values ***************************************|
\*******************************************************************************************************************/

byte hours = 0;
byte minutes = 0;
byte seconds = 0;
byte month = 0;
byte day = 0;
byte dayOfWeek = 0;
byte year = 0;

/*******************************************************************************************************************\
|****************************** Variables for Timer used to Read the DS3231 Clock Module ***************************|
\*******************************************************************************************************************/

unsigned long tickTime;					// Milliseconds variable for one second timer
unsigned long tickTimeOld;				// Old milliseconds holder used for comparison to tickTime
unsigned long INTERVAL = 1000;			// This is 1000 milliseconds = 1 second
unsigned long displayBlinkTime;			// Millisecond variable for the time / date display blink durations
unsigned long displayBlinkTimeOld = 0;

// Used in the 4 blink functions (Hours / Minutes / Months / Days)
int displayStateVisible = HIGH;			// Brightest display setting 
unsigned long blinkTime = 400;			// This is the duration of the display blinking when setting 
										// time and date


/*******************************************************************************************************************\
|*********************************** Define States for TIME & DATE State Machine ***********************************|
\*******************************************************************************************************************/

#define DISPLAY_TIME 0
#define DISPLAY_DATE 1
#define DISPLAY_BLINKING_HOURS 2
#define DISPLAY_BLINKING_MINUTES 3
#define DISPLAY_BLINKING_MONTHS 4
#define DISPLAY_BLINKING_DAYS 5

int TimeDateDisplayState = DISPLAY_TIME; // Initialize the top display state to "DISPLAY_TIME"

/* Array to keep stepper "seconds" values.  Since there are 4096 steps for one revolution, that does not easily divide
by 60 (4096/60seconds = 68.26667 steps per second for the analog seconds sweep hand. Through experimentation I found
that using a variable and incrementing it by 6 steps for each new position of the sweep hand, the hand didn't always
line up with the seconds "tick marks" on The Chronometer's faceplate.  I found that by loading the absolute number of
steps that the stepper motor needs to take into an array, that each position could be tweaked for correct alignment
and thereby eliminate the cumulative error caused by using a variable and incrementing by 68 steps each second.
Some experimentation and additional tweaking of these values might be required.
*/

int stepValues[60] =
{
	90,	// 1  second	
	180,	// 2  seconds	
	245,	// 3  seconds	
	310,	// 4  seconds	
	380,	// 5  seconds	
	440,	// 6  seconds	
	510,	// 7  seconds	
	580,	// 8  seconds	 
	650,	// 9  seconds	
	710,	// 10 seconds	 
	780,	// 11 seconds	 
	855,	// 12 seconds	 
	920,	// 13 seconds	 
	990,	// 14 seconds	 
	1050,	// 15 seconds	 
	1125,	// 16 seconds	  
	1185,	// 17 seconds	  
	1265,	// 18 seconds	  
	1320,	// 19 seconds	 
	1380,	// 20 seconds	 
	1444,	// 21 seconds
	1520,	// 22 seconds
	1580,	// 23 seconds
	1650,	// 24 seconds
	1720,	// 25 seconds
	1790,	// 26 seconds
	1853,	// 27 seconds
	1928,	// 28 seconds
	1990,	// 29 seconds
	2065,	// 30 seconds
	2130,	// 31 seconds
	2200,	// 32 seconds
	2270,	// 33 seconds
	2338,	// 34 seconds
	2409,	// 35 seconds
	2488,	// 36 seconds
	2546,	// 37 seconds
	2614,	// 38 seconds
	2682,	// 39 seconds
	2751,	// 40 seconds
	2829,	// 41 seconds
	2897,	// 42 seconds
	2965,	// 43 seconds
	3034,	// 44 seconds
	3102,	// 45 seconds
	3170,	// 46 seconds
	3234,	// 47 seconds
	3302,	// 48 seconds
	3370,	// 49 seconds
	3438,	// 50 seconds
	3507,	// 51 seconds
	3575,	// 52 seconds
	3643,	// 53 seconds
	3720,	// 54 seconds
	3785,	// 55 seconds
	3853,	// 56 seconds
	3921,	// 57 seconds
	4000,	// 58 seconds
	4065,	// 59 seconds
	4130	// 60 seconds
};

/*******************************************************************************************************************\
|************************************************* void Setup() ****************************************************|
\*******************************************************************************************************************/

void setup()
{
	readDS3231time(&seconds, &minutes, &hours, &dayOfWeek, &day, &month, &year);

	displayTime();

	// get "time" at the start of the program - used later to control 1 second tick
	tickTimeOld = millis();

	display.setColon(1);	// Turn on the colon on lower display
	tm1637.setColon(1);		// Turn on the colon between hours and minutes   

	Wire.begin();			// Start the I2C interface used to communicate with the DS3231 real time clock

// Setup for Chronograph
	chronoTimeOld = millis();					// Store Chrono start "time"
	display.showNumberDec(mins, true, 2, 0);	// Initialize mins display
	display.showNumberDec(sec, true, 2, 2);		// Initialize sec display


/*******************************************************************************************************************\
|*************************************** Initialize Button Count Variables *****************************************|
\*******************************************************************************************************************/

	ETButtonCount = 0;
	TimeDateButtonCount = 0;
	SetButtonCount = 0;
	PlusButtonCount = 0;
	MinusButtonCount = 0;
	ResetButtonCount = 0;
	CHRButtonCount = 0;

	/*******************************************************************************************************************\
	|************************************** Configure Arduino Pins For Pushbuttons *************************************|
	\*******************************************************************************************************************/

	/* Set Arduino Pins to inputs with an internal pull-up resistor.
		When a button is read it will:
		Return "1" / HIGH / TRUE if not pushed
		Return "0" / LOW / FALSE if pushed
	*/

	pinMode(btnET, INPUT_PULLUP);
	pinMode(btnTimeDate, INPUT_PULLUP);
	pinMode(btnSet, INPUT_PULLUP);
	pinMode(btnPlus, INPUT_PULLUP);
	pinMode(btnMinus, INPUT_PULLUP);
	pinMode(btnCHR, INPUT_PULLUP);
	pinMode(btnReset, INPUT_PULLUP);

	goToHomePosition();
	delay(500);
	Serial.begin(9600);

} // End of void setup()

/*******************************************************************************************************************\
|********************************************** void Loop() Function ***********************************************|
\*******************************************************************************************************************/

void loop()
{

	currentMillis = millis();   // capture the latest value of millis()
	displayBlinkTime = millis();

	// read the time from the module
	readDS3231time(&seconds, &minutes, &hours, &dayOfWeek, &day, &month, &year);

	checkButtons();				// Checks for Buttong Pushes
	TimeDisplayStateMachine();	// Starts the Time/Date state machine
	tickTime = millis();		// Used for Elapsed Time and Sweep Hand

	if (ETButtonCount == 1)
	{
		if (tickTime - tickTimeOld > INTERVAL)
		{
			tickTimeOld = tickTime;
			display.setColon(1);
			sec++; 		// Increment seconds

		// Check to see if 60 seconds have elapsed
			if (sec > 59) // if 60 sec, increment minutes
			{
				sec = 0;  // zero out the seconds
				mins++;   // increment the minutes

				if (mins > 59)  	// Check to see if 60 minutes have elapsed
				{
					mins = 0;     	// if yes, reset the minutes and start from 00:00 again
				}
			} // end of checking if 60 seconds have elapsed

			// Display minutes and seconds on Chronograph display
			display.showNumberDec(mins, true, 2, 0); //Displays Chronograph Minutes
			display.showNumberDec(sec, true, 2, 2);  //Displays Chronograph Seconds

			tickOneSecond();			// This starts the Sweep Second Hand

		}
	}
}

/*******************************************************************************************************************\
|******************************************** void checkButtons() Function *****************************************|
\*******************************************************************************************************************/

/* Buttons that have multiple functions based on the number of time pressed have an associated counter.
   Only the "+" and "-" buttons just get checked for press or no press.     All the buttons have an
   internal pull-up resistor on the input.   When a button is read it will return a "1" if not pushed
   and a "0" if pushed. The lines of code "if(!digitalRad(buttonName)" is looking for a button press
   and a "0" value
*/

void checkButtons()
{
	/*******************************************************************************************************************\
	|**************************************************** ET Button ****************************************************|
	\*******************************************************************************************************************/

	// This function has not yet been implemented

	/*
	Function: Starts and Pauses Analog Sweep Hand

	Possible Values:
		ETButtonCount = 0;  Starts Analog Sweep Hand
		ETButtonCount = 1;  Pauses Analog Sweep Hand
	*/

	//Read the ET (Elapse Time) Button
	if (ETbutton.update())
	{
		if (ETbutton.fallingEdge())
		{
			ETButtonCount++;
		}
	}
	if (ETButtonCount > 1)
	{
		ETButtonCount = 0;
		delay(5);
	}


	/*******************************************************************************************************************\
	|************************************************ TIME / DATE Button ***********************************************|
	\*******************************************************************************************************************/

	/*
	Function:  Toggles between the time and the date display

	Possible Values:
		TimeDateButtonCount = 0     Displays TIME
		TimeDateButtonCount = 1     Displays DATE
	*/

	// Read the TIME / DATE Button
	if (TimeDatebutton.update())
	{
		if (TimeDatebutton.fallingEdge())
		{
			TimeDateButtonCount++;
		}
	}
	if (TimeDateButtonCount > 1)
	{
		TimeDateButtonCount = 0;
		delay(5);
	}

	/*******************************************************************************************************************\
	|*************************************************** SET Button ****************************************************|
	\*******************************************************************************************************************/

	/*

	Function:  Used to change TIME (hours & minutes) and DATE (months & days) depending on what
	is being displayed at the time the button is pressed

	Possible Values:
		SetButtonCount = 0	Do Nothing

		SetButtonCount = 1	If state is DISPLAY_TIME - Transitions to DISPLAY_BLINKING_HOURS
					If state is DISPLAY_DATE - Transitions to DISPLAY_BLINKING_MONTHS

		SetButtonCount = 2	If state is DISPLAY_BLINKING_HOURS - Transitions to DISPLAY_BLINKING_MINUTES
					If state is DISPLAY_BLINKING_MONTHS - Transitions to DISPLAY_BLINKING_DAYS

		SetButtonCount = 3	If state is DISPLAY_BLINKING_MINUTES - Transitions to DISPLAY_TIME
					If state is DISPLAY_BLINKING_DAYS - Transitions to DISPLAY_DATE

		SetButtonCount > 3	SetButtonCount = 0
	*/

	// Read the SET Button
	if (Setbutton.update())
	{
		if (Setbutton.fallingEdge())
		{
			SetButtonCount++;
			delay(5);
		}
	}

	if (SetButtonCount > 3)
	{
		SetButtonCount = 0;
		delay(5);
	}

	/*******************************************************************************************************************\
	|*********************************************** + (Plus) Button ***************************************************|
	\*******************************************************************************************************************/

	/*
	Function:
	  Used to increment Hours / Minutes / Month and Date Depending on what is being displayed

	Possible Values:
	  PlusButtonCount = 0       Nothing
	  PlusButtonCount = 1       Increments Hour/Minutes/Month/Day Depending on what is being displayed
	  PlusButtonCount > 1       PlusButtonCount = 0
	*/

	// Read the "+" (Plus) Button

	/*
	Each time you press the Plus Button the PlusButtonCount variable is incremented.  The only states
	where the Plus button should be active are the ones shown below, when you try to set the time or
	date it will read the PlusButtonCount and increment the time or date on the display

		DISPLAY_BLINKING_HOURS		TimeDateDisplayState = 2
		DISPLAY_BLINKING_MINUTES	TimeDateDisplayState = 3
		DISPLAY_BLINKING_MONTHS		TimeDateDisplayState = 4
		DISPLAY_BLINKING_DAYS		TimeDateDisplayState = 5
	*/

	if (TimeDateDisplayState == 2 || TimeDateDisplayState == 3 || TimeDateDisplayState == 4 || TimeDateDisplayState == 5)
	{
		if (Plusbutton.update())
		{
			if (Plusbutton.fallingEdge())
			{
				PlusButtonCount = 1;
				delay(5);
			}
		}
	}

	/*******************************************************************************************************************\
	|************************************************** - (Minus) Button ***********************************************|
	\*******************************************************************************************************************/

	/*
	Function:
	  Used to increment Hours / Minutes / Month and Date Depending on what is being displayed

	Possible Values:
	  MinusButtonCount = 0      Do Nothing
	  MinusButtonCount = 1      Decrements Hour/Minutes/Month/Day Depending on what is being displayed
	  MinusButtonCount > 1      MinusButtonCount = 0
	*/

	/*
	Each time you press the Minus Button the MinusButtonCount variable is incremented.  The only states
	where the Plus button should be active are the ones shown below, when you try to set the time or
	date it will read the PlusButtonCount and decrement the time or date on the display

		DISPLAY_BLINKING_HOURS		TimeDateDisplayState = 2
		DISPLAY_BLINKING_MINUTES	TimeDateDisplayState = 3
		DISPLAY_BLINKING_MONTHS		TimeDateDisplayState = 4
		DISPLAY_BLINKING_DAYS		TimeDateDisplayState = 5
	*/

	if (TimeDateDisplayState == 2 || TimeDateDisplayState == 3 || TimeDateDisplayState == 4 || TimeDateDisplayState == 5)
	{
		// Read the "-" (Minus) Button
		if (Minusbutton.update())
		{
			if (Minusbutton.fallingEdge())
			{
				MinusButtonCount = 1; // needs to be cleared after going through state machine
				delay(5);
			}
		}
	}

	/*******************************************************************************************************************\
	|**************************************************** CHR Button ***************************************************|
	\*******************************************************************************************************************/

	/*
	Function:
	  Used to Toggles between Elapse Time and Chronograph Minutes

	Possible Values:
	  CHRButtonCount = 0    Nothing
	  CHRButtonCount = 1    Zeros out chronograph digital display & returns sweep hand to zero position
	  CHRButtonCount = 2    Starts chronograph digital display counting and sweep hand moving
	  CHRButtonCount = 3    Stops digital display and sweep hand
	  CHRButtonCount = 4    Restarts digital display counting and sweep hand moving
	  CHRButtonCount > 4    CHRButtonCount = 0
	*/

	// Read the "CHR" (Chronograph) Button
	if (CHRbutton.update())
	{
		if (CHRbutton.fallingEdge())
		{
			goToParkPosition(); // using this button to tesk "Parking" the sweep hand
			CHRButtonCount++;
		}
	}


	if (CHRButtonCount > 3)
	{
		CHRButtonCount = 0;
		delay(5);
	}

	/*******************************************************************************************************************\
	|************************************************** RESET Button ***************************************************|
	\*******************************************************************************************************************/

	/*
	Function:
	  Used to Reset Chronograph

	Possible Values:
	  ResetButtonCount = 0      Nothing
	  ResetButtonCount = 1      Sets Elapse Time back to 00:00 and Positions sweep hand to "15 minute" position
	  ResetButtonCount > 2      Reset ButtonCount = 0

	Note: When the analog Chronograph second sweep hand starts, it should position at the "15 Minute" position
	so that you can read the digital time and Chronograph displays
	*/

	// Read the "RESET" Button
	if (Resetbutton.update())
	{
		if (Resetbutton.fallingEdge())
		{
			sec = 0;
			mins = 0;
			display.showNumberDec(sec, true, 2, 0); //Displays Chronograph Minutes
			display.showNumberDec(mins, true, 2, 2); //Displays Chronograph Minutes
			ResetButtonCount = 1; // needs to be cleared after going through state machine
			ETButtonCount = 0; // reset the ET button count so that the Chronograph doesn't start counting
			goToHomePosition(); // return to the 12:00 position
			position = 100;  // get ready for the first "tick". Each second is 69 step of the motor
			displayTime();
			delay(5);
			secondsCounter = 0;

		}
	}
} // End of checkButtons()


/*******************************************************************************************************************\
|************************************** TimeDateDisplayState STATE MACHINE *****************************************|
\*******************************************************************************************************************/

void TimeDisplayStateMachine()
{

	switch (TimeDateDisplayState)
	{

		/*******************************************************************************************************************\
		|************************************************* State: DISPLAY_TIME *********************************************|
		\*******************************************************************************************************************/

	case DISPLAY_TIME:

		displayTime();      // Display the real-time clock data on the top display

		// When the TIME is being displayed, pushing the TIME/DATE button causes the DATE to be displayed
		if (TimeDateButtonCount == 1)
		{
			displayDate();
			TimeDateDisplayState = DISPLAY_DATE;
		}

		// When the TIME is being displayed, the 1st push of the "SET" button causes the HOURS to display
		if (SetButtonCount == 1)
		{
			TimeDateDisplayState = DISPLAY_BLINKING_HOURS;
			displayHours();
		}
		break;

		/*******************************************************************************************************************\
		|********************************************** State: DISPLAY_BLINKING_HOURS **************************************|
		\*******************************************************************************************************************/

	case DISPLAY_BLINKING_HOURS:

		// When the HOURS are blinking, pushing the "+" Button caused the Hours to be incremented
		if (PlusButtonCount == 1)
		{
			seconds = 0;				// Reset seconds
			hours++;				// Increment the hours
			if (hours > 12) hours = 1;		// The clock module is 24 hour based, adjust to 12 hour clock 
			// Write the updated hours to the DS3231 Clock Module
			setDS3231time(seconds, minutes, hours, dayOfWeek, day, month, year);
			blinkHours();
			delay(5);
			PlusButtonCount = 0;			// Clear the Button count
		}

		// When the HOURS are blinking, pushing the "-" Button caused the Hours to be decremented
		if (MinusButtonCount == 1)
		{
			seconds = 0;				// Reset seconds
			hours--;				// Decrement the hours
			if (hours < 1) hours = 12;		// The clock module is 24 hour based, adjust to 12 hour clock 

			// Write the updated hours to the DS3231 Clock Module
			setDS3231time(seconds, minutes, hours, dayOfWeek, day, month, year); // write to the module
			MinusButtonCount = 0;			// Clear the Button count
			delay(5);
		}
		blinkHours();
		// When the HOURS are being displayed, the 2nd push of the "SET" button causes just the MINUTES to display
		if (SetButtonCount == 2)
		{
			TimeDateDisplayState = DISPLAY_BLINKING_MINUTES;
			displayOff();
			displayMinutes();
		}
		break;

		/*******************************************************************************************************************\
		|******************************************* State: DISPLAY_BLINKING_MINUTES ***************************************|
		\*******************************************************************************************************************/

	case DISPLAY_BLINKING_MINUTES:

		// When the MINUTES are blinking, pushing the "+" Button caused the MINUTES to be incremented
		if (PlusButtonCount == 1)
		{
			seconds = 0;
			minutes++;
			if (minutes > 59) minutes = 1;
			setDS3231time(seconds, minutes, hours, dayOfWeek, day, month, year);
			delay(5);
			PlusButtonCount = 0;
		}

		// When the MINUTES are blinking, pushing the "-" Button caused the MINUTES to be decremented
		if (MinusButtonCount == 1)
		{
			seconds = 0;
			minutes--;
			if (minutes < 1) minutes = 59;
			setDS3231time(seconds, minutes, hours, dayOfWeek, day, month, year);
			delay(5);
			MinusButtonCount = 0;
		}
		blinkMinutes();

		// When the MINUTES are being displayed, the 3rd push of the "SET" button causes the Time to be displayed
		if (SetButtonCount == 3)
		{
			displayTime();
			TimeDateDisplayState = DISPLAY_TIME;
			SetButtonCount = 0; // Reset button count
		}
		break;

		/*******************************************************************************************************************\
		|*********************************************** State: DISPLAY_DATE ***********************************************|
		\*******************************************************************************************************************/

	case DISPLAY_DATE:

		// When the DATE is being displayed, pushing the TIME/DATE button causes the TIME to be displayed
		if (TimeDateButtonCount == 0)
		{
			displayDate();
			TimeDateDisplayState = DISPLAY_TIME;
		}

		// When the DATE is being displayed, the 1st push of the "SET" button causes the MONTHS to display      
		if (SetButtonCount == 1)
		{
			TimeDateDisplayState = DISPLAY_BLINKING_MONTHS;
			displayOff();
			displayMonths();
		}
		break;

		/*******************************************************************************************************************\
		|******************************************* State: DISPLAY_BLINKING_MONTHS ****************************************|
		\*******************************************************************************************************************/

	case DISPLAY_BLINKING_MONTHS:

		// When the MONTHS are blinking, pushing the "+" Button caused the MONTHS to be incremented
		if (PlusButtonCount == 1)
		{
			month++;
			if (month > 12) month = 1;
			setDS3231time(seconds, minutes, hours, dayOfWeek, day, month, year);
			delay(5);
			PlusButtonCount = 0;
		}

		// When the MONTHS are blinking, pushing the "-" Button caused the MONTHS to be decremented
		if (MinusButtonCount == 1)
		{
			month--;
			if (month < 1) month = 12;
			setDS3231time(seconds, minutes, hours, dayOfWeek, day, month, year);
			delay(5);
			MinusButtonCount = 0;
		}

		blinkMonths();

		// When the Months are blinking, the 2nd push of the "SET" button causes the DAYS to Blink  
		if (SetButtonCount == 2)
		{
			TimeDateDisplayState = DISPLAY_BLINKING_DAYS;
			displayDays();
		}
		break;

		/*******************************************************************************************************************\
		|******************************************* State: DISPLAY_BLINKING_DAYS ******************************************|
		\*******************************************************************************************************************/

	case DISPLAY_BLINKING_DAYS:

		// When the DAYS are blinking, pushing the "+" Button caused the DAYS to be incremented
		if (PlusButtonCount == 1)
		{
			day++;
			if (day > 31) day = 1;
			setDS3231time(seconds, minutes, hours, dayOfWeek, day, month, year);
			delay(5);
			//displayDays();
			PlusButtonCount = 0;
		}

		// When the DAYS are blinking, pushing the "-" Button caused the DAYS to be decremented
		if (MinusButtonCount == 1)
		{
			day--;
			if (day < 1) day = 31;
			setDS3231time(seconds, minutes, hours, dayOfWeek, day, month, year);
			delay(5);
			//displayOff();
			//displayDays();
			MinusButtonCount = 0;
		}

		blinkDays();

		// When the DAYS are blinking, pushing the "SET" Button caused the DATE to be displayed
		if (SetButtonCount == 3)
		{
			displayDate();
			TimeDateDisplayState = DISPLAY_DATE;
			SetButtonCount = 0; // Reset button count
		}
		break;

	} //End of switch(TimeDateDisplayState) statement

}// End of void TimeDisplayStateMachine() function

/*******************************************************************************************************************\
|************************************************* decToBcd() Function  ********************************************|
\*******************************************************************************************************************/

// The DS3231 Clock Module requires the information set to it be in BDC format

byte decToBcd(byte val) // For Clock - Convert normal decimal numbers to binary coded decimal (BCD)
{
	return((val / 10 * 16) + (val % 10));
}

/*******************************************************************************************************************\
|************************************************* bcdToDec() Function  ********************************************|
\*******************************************************************************************************************/

// The DS3231 Clock Module provides BCD data which needs to be converted to decimal to be sent to the displays

byte bcdToDec(byte val)  // For Clock - Convert binary coded decimal (BCD) to normal decimal numbers
{
	return((val / 16 * 10) + (val % 16));
}

/*******************************************************************************************************************\
|******************************************* setDS3231time() Function  *********************************************|
\*******************************************************************************************************************/

// This function allows you to set the DS3231 time, day of week and date

void setDS3231time(byte second, byte minute, byte hour, byte dayOfWeek, byte dayOfMonth, byte month, byte year)
{
	Wire.beginTransmission(DS3231_I2C_ADDRESS); // Start control communication to DS3231 Module
	Wire.write(0);                              // set next input to start at the seconds register
	Wire.write(decToBcd(second));               // set seconds
	Wire.write(decToBcd(minute));               // set minutes
	Wire.write(decToBcd(hour));                 // set hours
	Wire.write(decToBcd(dayOfWeek));            // set day of week (1=Sunday, 7=Saturday)
	Wire.write(decToBcd(dayOfMonth));           // set date (1 to 31)
	Wire.write(decToBcd(month));                // set month
	Wire.write(decToBcd(year));                 // set year (0 to 99)
	Wire.endTransmission();                     // Ends control communication to the DS3231 Module
}

/*******************************************************************************************************************\
******************************************* readDS3231time() Function  *********************************************|
\*******************************************************************************************************************/

// This funciton reads the time, day of week and date from DS3231

void readDS3231time(byte* second, byte* minute, byte* hour, byte* dayOfWeek, byte* dayOfMonth, byte* month, byte* year)
{
	Wire.beginTransmission(DS3231_I2C_ADDRESS); // Start control communication to DS3231 Module
	Wire.write(0);                              // set next input to start at the seconds register (00h)
	Wire.endTransmission();                     // Ends control communication to the DS3231 Module
	Wire.requestFrom(DS3231_I2C_ADDRESS, 7);    // Requests 7 items, sec, min, hour, doW, DoM, mon, year

// request seven bytes of data from DS3231 starting from register 00h

// converts the bcd data from DS3231 to decimal that will be sent to the display
	*second = bcdToDec(Wire.read() & 0x7f);
	*minute = bcdToDec(Wire.read());
	*hour = bcdToDec(Wire.read() & 0x3f);
	*dayOfWeek = bcdToDec(Wire.read());
	*dayOfMonth = bcdToDec(Wire.read());
	*month = bcdToDec(Wire.read());
	*year = bcdToDec(Wire.read());
}

/*******************************************************************************************************************\
****************************************** void displayTime() Function  ********************************************|
\*******************************************************************************************************************/

void displayTime()
{
	tm1637.setColon(1);   				// Turn on the colon between hours and minutes  

	if (hours > 12) hours = 1;		// The clock module is 24 hour based, adjust to 12 hour clock 
	if (hours < 1) hours = 12;		// The clock module is 24 hour based, adjust to 12 hour clock 

	tm1637.showNumberDec(hours, false, 2, 0);   	// Displays Hours without leading zeros
	tm1637.showNumberDec(minutes, true, 2, 2);    	// Displays Minutes with Leading zeros
}

/*******************************************************************************************************************\
****************************************** void displayDate() Function  ********************************************|
\*******************************************************************************************************************/

void displayDate()
{
	tm1637.setColon(0); // Turn off colon
	tm1637.showNumberDec(month, false, 2, 0); //Displays Month
	tm1637.showNumberDec(day, true, 2, 2);  //Displays Day
}

/*******************************************************************************************************************\
*************************************** void displayAllSegmentsOn() Function  **************************************|
\*******************************************************************************************************************/

void displayAllSegmentsOn()
{
	//To turn on all segments, use this command.
	byte data[] = { 0xff, 0xff, 0xff, 0xff }; tm1637.setSegments(data);
}

/*******************************************************************************************************************\
******************************************* void displayOff() Function  ********************************************|
\*******************************************************************************************************************/

void displayOff()
{
	// To turn off all segments, use this command.
	byte data[] = { 0x0, 0x0, 0x0, 0x0 }; tm1637.setSegments(data);
}

/*******************************************************************************************************************\
***************************************** void displayHours() Function  ********************************************|
\*******************************************************************************************************************/

void displayHours()
{
	// Function: (hours | false = do not display Leading zeros | 2 = two digits | 0 = Left most digit)
	displayOff();
	if (hours > 12) hours = 1;		// The clock module is 24 hour based, adjust to 12 hour clock 
	if (hours < 1) hours = 12;		// The clock module is 24 hour based, adjust to 12 hour clock 
	tm1637.showNumberDec(hours, false, 2, 0);       // Displays HOURS
}

/*******************************************************************************************************************\
***************************************** void displayMinutes() Function  ******************************************|
\*******************************************************************************************************************/

void displayMinutes()
{
	// Function: (minutes | false = display Leading zeros | 2 = two digits | 2 = third digit)
	tm1637.showNumberDec(minutes, true, 2, 2);  // Displays MINUTES
}

/*******************************************************************************************************************\
**************************************** void displayMonths() Function  ********************************************|
\*******************************************************************************************************************/

void displayMonths()
{
	// Function: (hours | false = do not display Leading zeros | 2 = two digits | 0 = Left most digit)
	displayOff();
	tm1637.showNumberDec(month, false, 2, 0);       // Displays Month
}

/*******************************************************************************************************************\
******************************************* void displayDays() Function  *******************************************|
\*******************************************************************************************************************/

void displayDays()
{
	// Function: (hours | false = do not display Leading zeros | 2 = two digits | 2 = third digit)
	displayOff();
	tm1637.showNumberDec(day, true, 2, 2);       	// Displays DAYS
}

/*******************************************************************************************************************\
******************************************* void displayChrono() Function  *****************************************|
\*******************************************************************************************************************/

void displayChrono()
{
	// Check to see if 1/2 second has elapsed
	if (chronoTime - chronoTimeOld > 496)
	{
		chronoTimeOld = chronoTime;       // store new "time"
		display.setColon(tick);           // turn on the colon

		// Check to see if the colon is on
		if (tick)  // colon ON = increment seconds
		{
			sec++; // Increment seconds

			// Check to see if 60 seconds have elapsed
			if (sec > 59) // if 60 sec, increment minutes
			{
				sec = 0;  // zero out the seconds
				mins++;   // increment the minutes

				if (mins > 59)  // Check to see if 60 minutes have elapsed
				{
					mins = 0;     // if yes, reset the minutes and start from 00:00 again
				}

				display.showNumberDec(mins, true, 2, 0); //Displays Chronograph Minutes

			} // end of checking if 60 seconds have elapsed

			display.showNumberDec(sec, true, 2, 2);  //Displays Chronograph Seconds
		}

		tick = !tick;             // toggle colon on/off

	} // End of Chronograph Code
}// end of Chronograph

/*******************************************************************************************************************\
************************************** void goToHomePosition() Function  *******************************************|
\*******************************************************************************************************************/

/*
Function's Purpose - Move the Stepper Motor Counter Clockwise until the Hall Effect Sensor detects the magnet
mounted to the plastic wheel.

Notes:  This function uses the setAcceleration method for the AccelStepper.h library so that it decelerates
as it approaches the Hall Effect Sensor so that it doesn't overshoot the home position.  The AccelStepper.h
library is used because it is non-blocking, meaning that the Arduino can perform other tasks while
the stepper motor is being moved.
*/

void goToHomePosition()
{
	// Set Max Speed and Acceleration of the Stepper Motor at startup for homing
	chronoStepper.setMaxSpeed(2000.0);				// Set Max Speed of Stepper 
	chronoStepper.setAcceleration(1000.0);			// Set Acceleration of Stepper

	// Make the Stepper move CCW until the switch is activated   
	while (digitalRead(home_switch))
	{
		chronoStepper.moveTo(initial_homing);		// Set the position to move to
		initial_homing--;             				// Decrease by 1 for next move if needed
		chronoStepper.run();          				// Start moving the stepper
	}

	// Once the stepper reaches the home position..
	chronoStepper.setCurrentPosition(0);         	// Set the current position as zero for now
	chronoStepper.setMaxSpeed(1000.0);           	// Set Max Speed of Stepper (Slower to get better accuracy)
	chronoStepper.setAcceleration(1000.0);       	// Set Acceleration of Stepper
	initial_homing = 1;


	while (!digitalRead(home_switch))
	{ // Make the Stepper move CW until the switch is deactivated
		chronoStepper.moveTo(initial_homing);
		chronoStepper.run();
		initial_homing++;

		delay(5);
	}

}

/*******************************************************************************************************************\
************************************** void goToParkPosition() Function  *******************************************|
\*******************************************************************************************************************/

/*
Function's Purpose - Move the Stepper Motor Clockwise until the seconds sweep hand is pointing to the
"3 o'clock" position. This is done so the sweep hand does not block viewing the two digital display when
the chronograph timer is not beign used.  This function should be called after the goToHomePosition() function.
*/

void goToParkPosition()
{
	// Set Max Speed and Acceleration of the Stepper Motor at startup for homing
	chronoStepper.setMaxSpeed(2000.0);	// Set Max Speed of Stepper 
	chronoStepper.setAcceleration(1000.0);	// Set Acceleration of Stepper
	chronoStepper.runToNewPosition(3102); // this is the "9:00" position

	delay(5);
}



/*******************************************************************************************************************\
**************************************** void tickOneSecond() Function  ********************************************|
\*******************************************************************************************************************/

/*
Function's Purpose: Move the Stepper Motor Counter clockwise one "second" at at time to simulate the motion
of an analog clock's second hand

Notes:  This function DOES NOT use the setAcceleration method for the AccelStepper.h library in order to had
the ticks to move quickly and snap to each second position.  The AccelStepper.h library is used
because it is non-blocking, meaning that the Arduino can perform other tasks while the stepper motor
is being moved.

The 28-BYJ-48 5volt stepper has internal gear reduction and takes 4096 steps to move 1 full revolution.
The 4096 steps divide by 60 second = 68 steps that need to be moved for each second "tick" of the stepper
motor.
*/

/*******************************************************************************************************************\
****************************************** void tickOneSecond() Function  ******************************************|
\*******************************************************************************************************************/

/*
Function's Purpose: Move the Stepper Motor Counter clockwise one "second" at at time to simulate the motion
of an analog clock's second hand
Notes:  This function DOES NOT use the setAcceleration method for the AccelStepper.h library in order to had
the ticks to move quickly and snap to each second position.  The AccelStepper.h library is used
because it is non-blocking, meaning that the Arduino can perform other tasks while the stepper motor
is being moved.
The 28-BYJ-48 5volt stepper has internal gear reduction and takes 4096 steps to move 1 full revolution.
The 4096 steps divide by 60 second = 68 steps that need to be moved for each second "tick" of the stepper
motor.
*/

void tickOneSecond()
{
	chronoStepper.moveTo(position);     // 4096 steps per revolution / 60 seconds = 68 steps for 1 second "tick"
	chronoStepper.setSpeed(3000);       // Set faster speed than what was used for homing

/*  The .distanceToGo() Method of the AccelStepper.h library keep track of the current position of the stepper motor
	relative to the distance sent via the .moveTo() method.  The position variable is initally set to 0 at the
	beginning of this program so that the first call of this function doesn't move the motor.  The first time
	the motor moves is the second time it goes through the while() loop. The while() loop checks the distanceToGo
	each pass and when it is equal to 0 (the stepper has moved one second)
	it increments the position variable by 68 steps to allow it to move another second.
*/

	while (chronoStepper.distanceToGo() != 0)
	{
		chronoStepper.runSpeedToPosition();

	} // end of while();

	secondsCounter++;

	position = stepValues[secondsCounter];

	if (secondsCounter == 60)
	{
		secondsCounter = 0;

		chronoStepper.setCurrentPosition(32); // estimate of position after homing sequence
		position = 110;
	}

}

/*******************************************************************************************************************\
******************************************** void blinkHours() Function  *******************************************|
\*******************************************************************************************************************/

void blinkHours()
{
	displayBlinkTime = millis();

	if (displayBlinkTime - displayBlinkTimeOld > blinkTime) // Check to see if blinkTime has elapsed
	{
		displayBlinkTimeOld = displayBlinkTime;

		if (displayStateVisible == LOW)
		{
			displayOff();
			displayStateVisible = HIGH;
		}
		else
		{
			displayHours();
			displayStateVisible = LOW;
		}
	}
}

/*******************************************************************************************************************\
******************************************* void blinkMinutes() Function  ******************************************|
\*******************************************************************************************************************/

void blinkMinutes()
{
	displayBlinkTime = millis();

	if (displayBlinkTime - displayBlinkTimeOld > blinkTime) // Check to see if blinkTime has elapsed
	{
		displayBlinkTimeOld = displayBlinkTime;

		if (displayStateVisible == LOW)
		{
			displayOff();
			displayStateVisible = HIGH;
		}
		else
		{
			displayMinutes();
			displayStateVisible = LOW;
		}
	}
}

/*******************************************************************************************************************\
******************************************** void blinkMonths() Function  ******************************************|
\*******************************************************************************************************************/

void blinkMonths()
{
	displayBlinkTime = millis();

	if (displayBlinkTime - displayBlinkTimeOld > blinkTime) // Check to see if blinkTime has elapsed
	{
		displayBlinkTimeOld = displayBlinkTime;

		if (displayStateVisible == LOW)
		{
			displayOff();
			displayStateVisible = HIGH;
		}
		else
		{
			displayMonths();
			displayStateVisible = LOW;
		}

	}
}

/*******************************************************************************************************************\
*********************************************** void blinkDays() Function  *****************************************|
\*******************************************************************************************************************/

void blinkDays()
{
	displayBlinkTime = millis();

	if (displayBlinkTime - displayBlinkTimeOld > blinkTime) // Check to see if blinkTime has elapsed
	{
		displayBlinkTimeOld = displayBlinkTime;

		if (displayStateVisible == LOW)
		{
			displayOff();
			displayStateVisible = HIGH;
		}
		else
		{
			displayDays();
			displayStateVisible = LOW;
		}
	}

} 