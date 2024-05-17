

/************************ HEADERS ****************************************/

#include "task.h"
#include "asf.h"
#include "sio2host.h"
#include "modbus.h"
#include "sensors.h"


#if defined(ENABLE_NETWORK_FREEZER)
#include "pdsDataServer.h"
#include "wlPdsTaskManager.h"
#endif

#include "system_task_manager.h"
#include "system_init.h"
#include "sw_timer.h"
#if (BOARD == SAMR34_XPLAINED_PRO)
#include "edbg-eui.h"
#endif

#include "driver_rn4871.h"
#include "udc.h"
#include "udi_cdc.h"
#include "sleepmgr.h"

/************************** DEFINITIONS **********************************/

/************************** PROTOTYPES **********************************/
void ReadMacAddress(void);
SYSTEM_TaskStatus_t APP_TaskHandler(void);


static bool _usb_attached=false;
static bool _usb_open=false;

void my_callback_dtr_notify(bool set)
{
	_usb_open=set;
}

bool my_callback_cdc_enable(void)
{
	_usb_attached = true;
	return true;
}

void my_callback_cdc_disable(void)
{
	_usb_attached = false;
	_usb_open=false;
}

bool usb_port_open(void)
{
	return _usb_attached && _usb_open;
}

static uint8_t tbuf[256],rd_i,wr_i;

void rx_lora_data(uint16_t len, uint8_t *x)
{
	uint16_t i;
	uint8_t wr_n;
	
	i=0;
	while(len--)
	{
		wr_n = wr_i + 1;
		
		if(wr_n != rd_i)
		{
			tbuf[wr_i] = x[i++];
			wr_i=wr_n;
		}	
	}
}

bool rx_empty(void)
{
	if(rd_i == wr_i)
		return true;
	else
		return false;
}

uint8_t rx_peekc(void)
{
	if(rx_empty())
		return 0;
	
	return tbuf[rd_i];
}

uint8_t rx_getc(void)
{
	if(rx_empty())
		return 0;
	
	return tbuf[rd_i++];	
}

int main ( void )
{
	uint8_t cnt,t;
	bool first;
    bool freezer_enable = false;
	int t2;
	
	irq_initialize_vectors();

	system_init();
	delay_init();
	LED_On(LED0);
	
	// Initialize the sleep manager
	sleepmgr_init();

	cpu_irq_enable();

	SENSORS_init();
	
	RN4871_init();

	SENSORS_5V_CTRL(1);

	// Start USB stack to authorize VBus monitoring
	udc_start();

#if defined (ENABLE_LCD)
	LCD_Initialize();
#endif

	sio2host_init();

	SENSORS_MUXSEL(1);
	t='!';
	sio2host_tx(&t, 1);

	// Permanent address for the device is set in miwi_config.h and loaded in global variable myLongAddress
	// Define SYMBOL MACRO to populate myLongAddress from EDBG_EUI or from MODULE_EUI
#if (EDBG_EUI_READ == 1 || MODULE_EUI_READ == 1)
	ReadMacAddress();
#endif

	SystemTimerInit();

	RN4871_Setup("Cadlink");

    // Demo Start Message
    DemoOutput_Greeting();

	nvm_init(INT_FLASH);


#if defined(ENABLE_NETWORK_FREEZER)
	PDS_Init();

    demo_output_freezer_options();
    // User Selection to commission a network or use Freezer
    freezer_enable = freezer_feature();
#endif

	rd_i=wr_i=0;
	
    // Commission the network
    Initialize_Demo(freezer_enable);

	/* Initialize demo application */
    Stack_Init();

	SENSORS_MUXSEL(1);
	Modbus_initialise(1,8 );
	Modbus2_initialise(1, 8);

	first=true;
	
	LED_Off(LED0);
    while(1)
    {
		SYSTEM_RunTasks();

		Run_Demo();

		if(usb_port_open())
		{
			LED_On(LED0);
			
			Modbus(0);
			
			if((!rx_empty()) && udi_cdc_is_tx_ready())
			{
				if(udi_cdc_putc(rx_peekc()))
					rx_getc();
			}
				
		}
		else
		{
			LED_Off(LED0);
			
			Modbus2(0);

			if(!rx_empty())
			{		
				t=rx_peekc();
				
				if(sio2host_tx(&t, 1))
					rx_getc();
			}			
			
		}

//LED_Toggle(LED0);

    }
}

/*********************************************************************//*
 \brief      Application Task Handler
 ************************************************************************/
SYSTEM_TaskStatus_t APP_TaskHandler(void)
{
    Run_Demo();
    return SYSTEM_TASK_SUCCESS;
}

/*********************************************************************
* Function:         void ReadMacAddress()
*
* PreCondition:     none
*
* Input:		    none
*
* Output:		    Reads MAC Address from SAM R34 XPRO EDBG or from
*					WLR089U0 Internal Flash
* Side Effects:	    none
*
* Overview:		    Uses the MAC Address for addressing
*
* Note:
**********************************************************************/
void ReadMacAddress(void)
{
#if (BOARD == SAMR34_XPLAINED_PRO && defined(__SAMR34J18B__))
	// only applicable for SAM R34 Xpro with EDBG on-board
	uint8_t* peui64 = edbg_eui_read_eui64() ;
	for (uint8_t i = 0; i < MY_ADDRESS_LENGTH; i++)
	{
		myLongAddress[i] = peui64[MY_ADDRESS_LENGTH-i-1] ;
	}
#elif (defined(__WLR089U0__))
	// applicable for module with WLR089 Xpro board or custom board
	#define NVM_UID_ADDRESS   ((volatile uint16_t *)(0x0080400AU))
	uint8_t i = 0, j = 0 ;
	uint8_t peui64[8] ;
	for (i = 0; i < MY_ADDRESS_LENGTH; i += 2, j++)
	{
		peui64[i] = (NVM_UID_ADDRESS[j] & 0xFF) ;
		peui64[i + 1] = (NVM_UID_ADDRESS[j] >> 8) ;
	}
	for (i= 0; i < MY_ADDRESS_LENGTH; i++)
	{
		myLongAddress[i] = peui64[MY_ADDRESS_LENGTH-i-1] ;
	}
#endif
}


bool	Write_register(uint16_t address,uint16_t data)
{
	
	return true;
}


uint16_t Read_Holding_Register(uint16_t address)
{
	return 0;


}

uint16_t Read_Input_Register(uint16_t address)
{
	return 0;
}
