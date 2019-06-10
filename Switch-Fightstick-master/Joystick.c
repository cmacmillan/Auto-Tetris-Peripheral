/*
Nintendo Switch Fightstick - Proof-of-Concept

Based on the LUFA library's Low-Level Joystick Demo
	(C) Dean Camera
Based on the HORI's Pokken Tournament Pro Pad design
	(C) HORI

This project implements a modified version of HORI's Pokken Tournament Pro Pad
USB descriptors to allow for the creation of custom controllers for the
Nintendo Switch. This also works to a limited degree on the PS3.

Since System Update v3.0.0, the Nintendo Switch recognizes the Pokken
Tournament Pro Pad as a Pro Controller. Physical design limitations prevent
the Pokken Controller from functioning at the same level as the Pro
Controller. However, by default most of the descriptors are there, with the
exception of Home and Capture. Descriptor modification allows us to unlock
these buttons for our use.
*/

/** \file
 *
 *  Main source file for the posts printer demo. This file contains the main tasks of
 *  the demo and is responsible for the initial application hardware configuration.
 */

#include "../LUFA/LUFA/Drivers/Peripheral/Serial.h"
#include "Joystick.h"
//#include "Arduino-master\cores\esp8266\HardwareSerial.h"

extern const uint8_t image_data[0x12c1] PROGMEM;

// Main entry point.
int main(void)
{
	// We'll start by performing hardware and peripheral setup.
	SetupHardware();
	// We'll then enable global interrupts for our use.
	GlobalInterruptEnable();
	// Once that's done, we'll enter an infinite loop.
	Serial_Init(9600, false);
	//Serial_SendByte(0x11);
	//Serial_SendByte(0x55);
	//Serial_SendByte(0x55);
	//Serial_SendByte(0x66);
	//Serial_SendByte(0x66);
	//Serial_SendByte(0x66);
	//Serial_SendByte(0x67);
	for (;;)
	{
		// We need to run our task to process and deliver data for our IN and OUT endpoints.
		HID_Task();
		// We also need to run the main USB management task.
		USB_USBTask();
		//Serial.begin(9600);
		//Serial_TxString("Test String\r\n");
	}
}

// Configures hardware and peripherals, such as the USB peripherals.
void SetupHardware(void)
{
	// We need to disable watchdog if enabled by bootloader/fuses.
	MCUSR &= ~(1 << WDRF);
	wdt_disable();

	// We need to disable clock division before initializing the USB hardware.
	clock_prescale_set(clock_div_1);
	// We can then initialize our hardware and peripherals, including the USB stack.

#ifdef ALERT_WHEN_DONE
// Both PORTD and PORTB will be used for the optional LED flashing and buzzer.
#warning LED and Buzzer functionality enabled. All pins on both PORTB and \
PORTD will toggle when printing is done.
	DDRD = 0xFF; //Teensy uses PORTD
	PORTD = 0x0;
	//We'll just flash all pins on both ports since the UNO R3
	DDRB = 0xFF; //uses PORTB. Micro can use either or, but both give us 2 LEDs
	PORTB = 0x0; //The ATmega328P on the UNO will be resetting, so unplug it?
#endif
	// The USB stack should be initialized last.
	USB_Init();
}

// Fired to indicate that the device is enumerating.
void EVENT_USB_Device_Connect(void)
{
	// We can indicate that we're enumerating here (via status LEDs, sound, etc.).
}

// Fired to indicate that the device is no longer connected to a host.
void EVENT_USB_Device_Disconnect(void)
{
	// We can indicate that our device is not ready (via status LEDs, sound, etc.).
}

// Fired when the host set the current configuration of the USB device after enumeration.
void EVENT_USB_Device_ConfigurationChanged(void)
{
	bool ConfigSuccess = true;

	// We setup the HID report endpoints.
	ConfigSuccess &= Endpoint_ConfigureEndpoint(JOYSTICK_OUT_EPADDR, EP_TYPE_INTERRUPT, JOYSTICK_EPSIZE, 1);
	ConfigSuccess &= Endpoint_ConfigureEndpoint(JOYSTICK_IN_EPADDR, EP_TYPE_INTERRUPT, JOYSTICK_EPSIZE, 1);

	// We can read ConfigSuccess to indicate a success or failure at this point.
}

// Process control requests sent to the device from the USB host.
void EVENT_USB_Device_ControlRequest(void)
{
	// We can handle two control requests: a GetReport and a SetReport.

	// Not used here, it looks like we don't receive control request from the Switch.
}

typedef enum
{
	SYNC_CONTROLLER,
	READ_INPUT,
	//ENTER_FROM_MAIN_MENU,
	//SELECT_CHARACTER,
	//SYNC_POSITION,
	//STOP_X,
	//STOP_Y,
	//MOVE_X,
	//MOVE_Y,
	DONE,
} State_t;
State_t state = SYNC_CONTROLLER;

#define ECHOES 2
int echoes = 0;
USB_JoystickReport_Input_t last_report;

int report_count = 0;
int xpos = 0;
int ypos = 0;
int portsval = 0;

// Process and deliver data from IN and OUT endpoints.
void HID_Task(void)
{
	// If the device isn't connected and properly configured, we can't do anything here.
	if (USB_DeviceState != DEVICE_STATE_Configured)
		return;

	// We'll start with the OUT endpoint.
	Endpoint_SelectEndpoint(JOYSTICK_OUT_EPADDR);
	// We'll check to see if we received something on the OUT endpoint.
	if (Endpoint_IsOUTReceived())
	{
		// If we did, and the packet has data, we'll react to it.
		if (Endpoint_IsReadWriteAllowed())
		{
			// We'll create a place to store our data received from the host.
			// ---I commented this--- USB_JoystickReport_Output_t JoystickOutputData;
			uint8_t test[7];

			// We'll then take in that data, setting it up in our storage.
			// ---I commented this--- while (Endpoint_Read_Stream_LE(&JoystickOutputData, sizeof(JoystickOutputData), NULL) != ENDPOINT_RWSTREAM_NoError)
			while (Endpoint_Read_Stream_LE(&test, sizeof(test), NULL) != ENDPOINT_RWSTREAM_NoError)
				;
			// At this point, we can react to this data.
			/*if (JoystickOutputData.RY > 0 || JoystickOutputData.Button > 0)
			{
				state = SYNC_CONTROLLER;
				report_count = 0;
			}*/
			// However, since we're not doing anything with this data, we abandon it.
		}
		// Regardless of whether we reacted to the data, we acknowledge an OUT packet on this endpoint.
		Endpoint_ClearOUT();
	}

	/*uint8_t DataByte = Serial_RxByte();
	if (DataByte != 0)
	{
		state =
	}*/
	// We'll then move on to the IN endpoint.
	Endpoint_SelectEndpoint(JOYSTICK_IN_EPADDR);
	// We first check to see if the host is ready to accept data.
	if (Endpoint_IsINReady())
	{
		// We'll create an empty report.
		USB_JoystickReport_Input_t JoystickInputData;
		// We'll then populate this report with what we want to send to the host.
		GetNextReport(&JoystickInputData);
		// Once populated, we can output this data to the host. We do this by first writing the data to the control stream.
		while (Endpoint_Write_Stream_LE(&JoystickInputData, sizeof(JoystickInputData), NULL) != ENDPOINT_RWSTREAM_NoError)
			;
		// We then send an IN packet on this endpoint.
		Endpoint_ClearIN();
	}
}

int spinsToRight=0;
int movesToMake=0;
bool isPieceActive=false;
int framesToWait=0;
bool Lbutton=false;
const int FRAME_DELAY =3;
// Prepare the next report for the host.
void GetNextReport(USB_JoystickReport_Input_t *const ReportData)
{

	// Prepare an empty report
	memset(ReportData, 0, sizeof(USB_JoystickReport_Input_t));
	ReportData->LX = STICK_CENTER; //this just inits the sticks to 128
	ReportData->LY = STICK_CENTER;
	ReportData->RX = STICK_CENTER;
	ReportData->RY = STICK_CENTER;
	ReportData->HAT = HAT_CENTER;

	// Repeat ECHOES times the last report
	if (echoes > 0)
	{
		memcpy(ReportData, &last_report, sizeof(USB_JoystickReport_Input_t));
		echoes--;
		return;
	}
		//Serial_SendByte(0x01);

		//if (!isPieceActive)
		//{
			int16_t DataByte = Serial_ReceiveByte();
			if (DataByte != -1)
			{
				//while(Serial_ReceiveByte()!=-1);
				//10 possible rows so 4 bits
				//4 orientations so 2 bits
				//A button so 1 bit
				//L button so 1 big
				//So final byte is A button, L button,2 bits for orientation, 1 bit for direction, 3 bits for direction
				//orientation offsetSign mag
				if ((DataByte >> 7) & 1)
				{
					if (DataByte&1){
						ReportData->Button |= SWITCH_A;
					}
					if (DataByte&2){
						ReportData->RX = STICK_MIN;
					}
					if (DataByte&4){
						ReportData->RX = STICK_MAX;
					}
					if (DataByte&8){
						ReportData->RY = STICK_MIN;
					}
					if (DataByte&16){
						ReportData->RY = STICK_MAX;
					}
				}
				else
				{
					Lbutton = (DataByte & (1 << 6));
					movesToMake = (DataByte & 7);							   //3 magnitude
					movesToMake = (DataByte & 8) ? -movesToMake : movesToMake; // 1000
					spinsToRight = (DataByte >> 4) & 3;						   //3=11
					framesToWait = 5;
					isPieceActive = true;
				}
			}
		//}
		//else
		if (isPieceActive)
		{
			if (framesToWait > 0)
			{
				framesToWait--;
			}
			else
			{
				if (Lbutton)
				{
					ReportData->Button |= SWITCH_L;
					framesToWait = FRAME_DELAY;
					Lbutton = false;
				}
				else if (spinsToRight > 0)
				{
					if (spinsToRight == 3)
					{
						ReportData->Button |= SWITCH_X;
						framesToWait = FRAME_DELAY;
						spinsToRight = 0;
					}
					else
					{
						ReportData->Button |= SWITCH_Y;
						framesToWait = FRAME_DELAY;
						spinsToRight--;
					}
				}
				else if (movesToMake > 0)
				{
					ReportData->HAT = HAT_LEFT;
					framesToWait = FRAME_DELAY;
					movesToMake--;
				}
				else if (movesToMake < 0)
				{
					ReportData->HAT = HAT_RIGHT;
					framesToWait = FRAME_DELAY;
					movesToMake++;
				}
				else if (spinsToRight == 0)
				{
					ReportData->HAT = HAT_TOP;
					framesToWait = FRAME_DELAY;
					isPieceActive = false;
				}
			}
		}

		/*if (state != SYNC_CONTROLLER && state != SYNC_POSITION)
		if (pgm_read_byte(&(image_data[(xpos / 8) + (ypos * 40)])) & 1 << (xpos % 8))
			ReportData->Button |= SWITCH_A;*/

		// Prepare to echo this report
		memcpy(&last_report, ReportData, sizeof(USB_JoystickReport_Input_t));
		echoes = ECHOES;
}
