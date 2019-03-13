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
	ENTER_FROM_MAIN_MENU,
	SELECT_CHARACTER,
	//SYNC_POSITION,
	//STOP_X,
	//STOP_Y,
	//MOVE_X,
	//MOVE_Y,
	DONE
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
			USB_JoystickReport_Output_t JoystickOutputData;
			// We'll then take in that data, setting it up in our storage.
			while (Endpoint_Read_Stream_LE(&JoystickOutputData, sizeof(JoystickOutputData), NULL) != ENDPOINT_RWSTREAM_NoError)
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

	// States and moves management
	switch (state)
	{

	case SYNC_CONTROLLER:
		if (report_count > 100)
		{
			report_count = 0;
			//state = ENTER_FROM_MAIN_MENU; //SYNC_POSITION;
			state = SELECT_CHARACTER;
		}
		else if (report_count == 25 || report_count == 50)
		{
			ReportData->Button |= SWITCH_L | SWITCH_R;
		}
		else if (report_count == 75 || report_count == 100)
		{
			if (report_count % 2 == 0)
			{
				ReportData->Button |= SWITCH_A;
			}
		}
		report_count++;
		break;
	case ENTER_FROM_MAIN_MENU:
		if (report_count > 500)
		{
			report_count = 0;
			state = SELECT_CHARACTER;
			Serial_SendByte(0x11);
		}
		else
		{
			if (report_count % 2 == 0)
			{
				ReportData->Button |= SWITCH_A;
			}
		}
		report_count++;
		break;
	case SELECT_CHARACTER:
		/*if (report_count > 21)
		{
			//int report_countMod = report_count % 10;
			if (report_count % 2 == 0)
			{
				ReportData->LY = STICK_MAX;
			}
			else
			{
				ReportData->LY = STICK_CENTER;
			}
		}
		else*/
		if (report_count == 1000)
		{
			Serial_SendByte(0x12);
			ReportData->Button |= SWITCH_PLUS;
		}
		else if (report_count == 20)
		{
			ReportData->Button |= SWITCH_A;
		}
		else
		{
			ReportData->LY = STICK_MIN;
		}
		report_count++;
		break;
	/*case SYNC_POSITION:
		if (report_count == 250)
		{
			report_count = 0;
			xpos = 0;
			ypos = 0;
			state = STOP_X;
		}
		else
		{
			// Moving faster with LX/LY
			ReportData->LX = STICK_MIN;
			ReportData->LY = STICK_MIN;
		}
		if (report_count == 75 || report_count == 150)
		{
			// Clear the screen
			ReportData->Button |= SWITCH_MINUS;
		}
		report_count++;
		break;
	case STOP_X:
		state = MOVE_X;
		break;
	case STOP_Y:
		if (ypos < 120 - 1)
			state = MOVE_Y;
		else
			state = DONE;
		break;
	case MOVE_X:
		if (ypos % 2)
		{
			ReportData->HAT = HAT_LEFT;
			xpos--;
		}
		else
		{
			ReportData->HAT = HAT_RIGHT;
			xpos++;
		}
		if (xpos > 0 && xpos < 320 - 1)
			state = STOP_X;
		else
			state = STOP_Y;
		break;
	case MOVE_Y:
		ReportData->HAT = HAT_BOTTOM;
		ypos++;
		state = STOP_X;
		break;*/
	case DONE:
#ifdef ALERT_WHEN_DONE
		portsval = ~portsval;
		PORTD = portsval; //flash LED(s) and sound buzzer if attached
		PORTB = portsval;
		_delay_ms(250);
#endif
		return;
	}

	// Inking
	/*if (state != SYNC_CONTROLLER && state != SYNC_POSITION)
		if (pgm_read_byte(&(image_data[(xpos / 8) + (ypos * 40)])) & 1 << (xpos % 8))
			ReportData->Button |= SWITCH_A;*/

	// Prepare to echo this report
	memcpy(&last_report, ReportData, sizeof(USB_JoystickReport_Input_t));
	echoes = ECHOES;
}
