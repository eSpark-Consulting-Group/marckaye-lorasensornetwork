/**
 * \file
 *
 *
 (c) 2018 Microchip Technology Inc. and its subsidiaries.

    Subject to your compliance with these terms,you may use this software and
    any derivatives exclusively with Microchip products.It is your responsibility
    to comply with third party license terms applicable to your use of third party
    software (including open source software) that may accompany Microchip software.

    THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER
    EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY IMPLIED
    WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS FOR A
    PARTICULAR PURPOSE.

    IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE,
    INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND
    WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS
    BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO THE
    FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN
    ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY,
    THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
 *
 */

#include "driver_rn4871.h"
#include <string.h>
#include <sio2host.h>
#include "sensors.h"

#define PIN_BLE_RESET PIN_PB23
#define PIN_BLE_RXIND PIN_PB22
#define PIN_BLE_LINK  PIN_PA27
#define PIN_BLE_RSSI  PIN_PA08
#define PIN_BLE_STAT  PIN_PA07

typedef enum { NO_RESPONSE, OK } rn4871_Responses_t;

#define RN4871_DELAY                            40

volatile uint16_t rn_index = 0;
volatile uint8_t  rn_buffer[RN_BUFF_SIZE];

//RN4871 commands
//static const char rn4871Command_commandMode[] = {"$$$"};
//static const char rn4871Command_setServices[] = {"ss,c0"};
//static const char rn4871Command_getServices[] = {"gs"};
//static const char rn4871Command_listCharacteristics[] = {"ls"};
//static const char rn4871Command_createSensorService[] = {"ps,"};
//static const char rn4871Command_createCharacteristic[] = {"pc,"};
//static const char rn4871Reply_newCommand[] = {"CMD> "};

//static const char endOfLine[] = {"\r\n\0"};
//static const char mySetServices[] = {"80"};
//static const char characteristicAttributes[] = {",1a,14"};

void RN4871_init(void)
{
	struct port_config pin_conf;

	// Set outputs to benign states
	port_pin_set_output_level(PIN_BLE_RESET,1);
	port_pin_set_output_level(PIN_BLE_RXIND,1);
	port_pin_set_output_level(PIN_BLE_LINK,1);

	/* Configure the GPIO pins as outputs*/
	port_get_config_defaults(&pin_conf);
	pin_conf.direction  = PORT_PIN_DIR_OUTPUT;
	port_pin_set_config(PIN_BLE_RESET, &pin_conf);

	port_get_config_defaults(&pin_conf);
	pin_conf.direction  = PORT_PIN_DIR_OUTPUT;
	port_pin_set_config(PIN_BLE_RXIND, &pin_conf);

	port_get_config_defaults(&pin_conf);
	pin_conf.direction  = PORT_PIN_DIR_OUTPUT;
	port_pin_set_config(PIN_BLE_LINK, &pin_conf);

	// Set inputs
	port_get_config_defaults(&pin_conf);
	pin_conf.direction  = PORT_PIN_DIR_INPUT;
	port_pin_set_config(PIN_BLE_STAT, &pin_conf);

	port_get_config_defaults(&pin_conf);
	pin_conf.direction  = PORT_PIN_DIR_INPUT;
	port_pin_set_config(PIN_BLE_RSSI, &pin_conf);

	SENSORS_MUXSEL(0);
}

void RN4871_link_drop(void)
{
	port_pin_set_output_level(PIN_BLE_LINK,0);
	delay_ms(20);
	port_pin_set_output_level(PIN_BLE_LINK,1);	
}

uint8_t RN4871_low_rssi(void)
{
	return port_pin_get_input_level(PIN_BLE_RSSI);
}

uint8_t RN4871_attached(void)
{
	if(!port_pin_get_input_level(PIN_BLE_STAT))
		return true;
	else
		return false;
}

void rn4871_SendString(const char *command)
{
	rn4871_SendBuffer(command, strlen(command));
}

void rn4871_SendBuffer(const char *buffer, uint8_t length)
{
	while (length--)
		rn4871_SendByte(*buffer++);
}

void rn4871_SendByte(uint8_t data)
{
	sio2host_putchar(data);
}

static void RN4871_CaptureReceivedMessage(void)
{
	int data = sio2host_getchar_nowait();

	if(data < 0) return;

	if (rn_index < RN_BUFF_SIZE)
	rn_buffer[rn_index++] = (uint8_t)data;
}

void RN4871_Setup(const char *name)
{
	char nameString[32];

	SENSORS_MUXSEL(0);

	RN4871_Reset_Module();
	RN4871_sendAndWait("$$$", "CMD> ", 40);
	RN4871_sendAndWait("SS,C0\r\n", "AOK\r\n", 40);			// uart and device information
	RN4871_sendAndWait("SR,00000000\r\n", "AOK\r\n", 40);	// disable all features
	//RN4871_sendAndWait("PS,180F\r\n", "AOK\r\n", 40);		
	//RN4871_sendAndWait("PC,2A19,10,05\r\n", "AOK\r\n", 40);
	sprintf(nameString, "S-,%s\r\n", name);
	RN4871_sendAndWait(nameString, "AOK\r\n", 40);
	RN4871_sendAndWait("R,1\r\n", "Rebooting", 40);
	RN4871_sendAndWait("$$$", "CMD> ", 40);
	RN4871_sendAndWait("A\r\n", "AOK\r\n", 40);
	RN4871_sendAndWait("SO,1\r\n", "AOK\r\n", 40);			// deep sleep
	RN4871_sendAndWait("---\r\n", "END", 40);
	RN4871_ClearReceivedMessage();
}

void RN4871_Reset_Module(void)
{
	port_pin_set_output_level(PIN_BLE_RESET,0);

	RN4871_blockingWait(1);
	
	port_pin_set_output_level(PIN_BLE_RESET,1);
	
	RN4871_blockingWait(50);
}

void RN4871_ClearReceivedMessage(void)
{
	memset((void *)rn_buffer, 0, RN_BUFF_SIZE);
	rn_index = 0;
}

uint8_t RN4871_CheckResponse(const char *response)
{
	uint8_t ret = 0;
	if (strstr((const char *)rn_buffer, response))
	ret = 1;
	return ret;
}

void RN4871_blockingWait(uint16_t limit)
{
	for (uint16_t counter = 0; counter < limit*15; counter++)
	{
		RN4871_CaptureReceivedMessage();
		delay_ms(1);
	}
}
void RN4871_sendAndWait(const char *sendString, const char *response, uint16_t delay)
{
	do {
		RN4871_ClearReceivedMessage();
		rn4871_SendString(sendString);
		RN4871_blockingWait(delay);
	} while (RN4871_CheckResponse(response) == NO_RESPONSE);
}

#if 0

static void RN4871_enterCommandMode(void)
{
	RN4871_sendAndWait(rn4871Command_commandMode, rn4871Reply_newCommand, RN4871_DELAY);
}


static void updateRN4871Services(void)
{
	uint8_t rn4871Response = 0;

	RN4871_ClearReceivedMessage();

	rn4871_SendString(rn4871Command_getServices);
	rn4871_SendString(endOfLine);
	RN4871_blockingWait(RN4871_DELAY);

	rn4871Response = RN4871_CheckResponse(mySetServices);
	if (!rn4871Response)
	{
		rn4871_SendString(rn4871Command_setServices);
		rn4871_SendString(mySetServices);
		rn4871_SendString(endOfLine);
		RN4871_blockingWait(RN4871_DELAY);
	}
}

#endif
