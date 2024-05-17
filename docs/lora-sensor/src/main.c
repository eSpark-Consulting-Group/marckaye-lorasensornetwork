

/************************ HEADERS ****************************************/

// Define this to ignore setting up the LMP91000
#define IGNORE_LMP

#include "task.h"
#include "asf.h"
#include "sio2host.h"
#include "modbus.h"
#include "gps.h"

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

#include "ssd1306xled.h"
#include "i2c1.h"
#include "i2c2.h"
#include "sensors.h"
#include "I2CEEPROM.h"
#include "driver_rn4871.h"
#include "udc.h"
#include "udi_cdc.h"
#include "sleepmgr.h"
#include "LSM6.h"
#include "tcc.h"
#include "tcc_callback.h"


/************************** DEFINITIONS **********************************/

typedef uint32_t XEE_ADDR;

#define LOGCHANNELS 14u
#define LOGDUPLICATES 1u

uint8_t clockset_sec, clockset_min, clockset_hr, clockset_day, clockset_mon;
uint16_t clockset_yr;

uint16_t sleep_period=0;
static bool update_sleep_period=false;


uint16_t day_index=0;

uint32_t entry,last_entry,start_entry;

uint8_t logging_state=0;

uint8_t servo=0;

uint8_t heaters=1;

/************************** PROTOTYPES **********************************/
void ReadMacAddress(void);
SYSTEM_TaskStatus_t APP_TaskHandler(void);

static volatile uint8_t cb_counter=0;

static volatile struct rtc_calendar_time rtc_time,clockset;

static bool load_clk=false;


U32 get_ticker(void);

static int days_in_month(int month, int year)
{
	int days;
	if (month == 4 || month == 6 || month == 9 || month == 11) {
		days = 30;
		} else if (month == 2) {
		if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0)) {
			days = 29;
			} else {
			days = 28;
		}
		} else {
		days = 31;
	}
	return days;
}

static void rtc_get_time(struct rtc_calendar_time *p)
{
	if(p!=NULL)
	*p=rtc_time;
}

static volatile U32 ticker;


U32 get_ticker(void)
{	
	return ticker;
}

// one second tick
static void rtc_callback(struct tcc_module *const module_inst)
{
	static U8 prescaler=50;

	ticker++;
		
	if(load_clk)
	{
		rtc_time = clockset;
		load_clk=false;
	}

	if(prescaler)
	{
		prescaler--;
		return;
	}
	
	prescaler=50;
	
	cb_counter++;

	if(rtc_time.second < 59)
	rtc_time.second++;
	else
	{
		rtc_time.second=0;
		if(rtc_time.minute < 59)
		rtc_time.minute++;
		else
		{
			rtc_time.minute=0;
			if(rtc_time.hour < 23)
			rtc_time.hour++;
			else
			{
				rtc_time.hour=0;

				if(rtc_time.day < days_in_month(rtc_time.month, rtc_time.year))
				rtc_time.day++;
				else
				{
					rtc_time.day=1;
					if(rtc_time.month < 12)
					rtc_time.month++;
					else
					{
						rtc_time.month=1;
						rtc_time.year++;
					}
				}
			}
		}

	}
}

static struct tcc_module tcc_test0_module;
static struct tcc_config tcc_test0_config;

#define CONF_PWM_PIN_OUT         PIN_PA18F_TCC0_WO2
#define CONF_PWM_PIN_MUX         MUX_PA18F_TCC0_WO2

static void servo_position(int degrees)
{
	uint32_t value;

	if(degrees < 0) degrees=0;
	else if(degrees > 90) degrees=90;

	// 0.5ms / 20ms = 90 degree span
	value  = 24000;
	value *= degrees;
	value /= 90;

	// 0 degrees = 1.5ms / 20ms
	value += 72000;

	tcc_set_compare_value(&tcc_test0_module, TCC_MATCH_CAPTURE_CHANNEL_2, value);
}


static void servo_init(void)
{
	tcc_reset(&tcc_test0_module);
	tcc_get_config_defaults(&tcc_test0_config, TCC0);
	tcc_test0_config.counter.period =  960000;	// 50Hz with a 48MHz clock
	tcc_test0_config.compare.wave_generation  = TCC_WAVE_GENERATION_SINGLE_SLOPE_PWM;
	tcc_test0_config.compare.match[TCC_MATCH_CAPTURE_CHANNEL_2]  = 72000;	// 1.5ms = 0 degrees 1.75ms = 45 degrees 2.0ms = 90 degrees

	/* Calculate the theoretical PWM frequency & duty */
	//uint32_t frequency_output, duty_output;
	//frequency_output = system_clock_source_get_hz(CONF_CLOCK_GCLK_0_CLOCK_SOURCE) / (960000+1);

	/* This value is depend on the WaveGeneration Mode */
	//duty_output = (uint32_t)(tcc_test0_config.compare.match[TCC_MATCH_CAPTURE_CHANNEL_0] * 100) / tcc_test0_config.counter.period;

	tcc_test0_config.pins.enable_wave_out_pin[TCC_MATCH_CAPTURE_CHANNEL_2] = true;
	tcc_test0_config.pins.wave_out_pin[TCC_MATCH_CAPTURE_CHANNEL_2]        = CONF_PWM_PIN_OUT;
	tcc_test0_config.pins.wave_out_pin_mux[TCC_MATCH_CAPTURE_CHANNEL_2]    = CONF_PWM_PIN_MUX;
	tcc_test0_config.run_in_standby = true;
	tcc_init(&tcc_test0_module, TCC0, &tcc_test0_config);
	tcc_enable(&tcc_test0_module);
	
	tcc_register_callback(&tcc_test0_module, rtc_callback, TCC_CALLBACK_CHANNEL_2);
	tcc_enable_callback(&tcc_test0_module, TCC_CALLBACK_CHANNEL_2);
	
}

static void UpdateServo(void)
{
	if(!servo)
		servo_position(0);
	else
		servo_position(60);
}

static void rtc_init(void)
{

	clockset.year=2023;
	clockset.month=5;
	clockset.day=22;
	clockset.hour=19;
	clockset.minute=38;
	clockset.second=0;

	load_clk=true;

}

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

static bool usb_port_open(void)
{
	return _usb_attached && _usb_open;
}

static void XEEReadArray(XEE_ADDR _addr, uint8_t *_array, uint8_t len)
{
	uint8_t i;

	for(i=0;i<len;i++)
	{
		_array[i]=I2CEEPROM_read(_addr+i);
	}
}

static void XEEWriteArray(XEE_ADDR _addr, uint8_t *_array, uint8_t len)
{
	uint8_t i;

	for(i=0;i<len;i++)
	{
		I2CEEPROM_write(_addr+i,_array[i]);
	}
}

static bool ReadSleepPeriod(void)
{
	uint8_t i;

	for(i=0;i<10;i++)
	{
		XEEReadArray(0, (void *)&sleep_period, 2u);

		if((sleep_period < 1) || (sleep_period > 2880))
			delay_ms(10);
		else
			break;
	}

	if(i == 10u)
	{
		sleep_period=120;	// default to 2 hrs if stored value is invalid
		return false;
	}
	else
		return true;
}

static bool WriteSleepPeriod(void)
{
	uint16_t t;
	int i;

	for(i=0;i<10;i++)
	{
		XEEWriteArray(0, (void *)&sleep_period, 2u);

		XEEReadArray(0, (void *)&t, 2u);

		if(sleep_period == t)
			return true;

		delay_ms(10);
	}

	return false;
}

uint16_t measurements[LOGCHANNELS];

XEE_ADDR erase_address=0;

uint32_t erase_timer;

static void BackgroundErase(void)
{
	
	if(!erase_address)
		return;

	if(ticker == erase_timer)
		return;
	
	while(erase_address++ < 0x20000L)
	{

		if(I2CEEPROM_read(erase_address)!=0xFF)
		{
			I2CEEPROM_write(erase_address,0xFF);
			break;
		}
	
		if(!(erase_address & 0xFF))
		{
			LED_Toggle(LED0);
			break;
		}
		
	}
	
	erase_timer = ticker;

	// check for end
	if(erase_address >= 0x20000L)
		erase_address=0;
}

bool LogData(uint16_t *m, uint8_t offset);

// Turn heaters on or off
static void UpdateHeaters(void)
{
	SENSORS_3V3_SENS(heaters);
	SENSORS_5V_CTRL(heaters);
}

static void RunLogger(void)
{
	static uint16_t msave1[LOGCHANNELS],msave2[LOGCHANNELS],msave3[LOGCHANNELS];
	uint32_t accum1,accum2,accum3;
	uint8_t i;

	// process logging machine
	switch(logging_state)
	{
		// idle
		default:
		case 0:
			break;

		// started .. give transducers a chance to power up
		case 1:
	
			logging_state++;
			break;

		case 2:
			for(i=0;i<LOGCHANNELS;i++)
				msave1[i]=measurements[i];
			logging_state++;
			break;

		case 3:
			for(i=0;i<LOGCHANNELS;i++)
				msave2[i]=measurements[i];
			logging_state++;
			break;

		case 4:
			for(i=0;i<LOGCHANNELS;i++)
				msave3[i]=measurements[i];
			logging_state++;
			break;

		case 5:

			// pick the best
			accum1=0;
			accum2=0;
			accum3=0;
			for(i=0;i<LOGCHANNELS;i++)
			{
				if(msave1[i] > msave2[i]) accum1 += msave1[i]-msave2[i]; else accum1 += msave2[i]-msave1[i];
				if(msave2[i] > msave3[i]) accum2 += msave2[i]-msave3[i]; else accum2 += msave3[i]-msave2[i];
				if(msave3[i] > msave1[i]) accum3 += msave3[i]-msave1[i]; else accum3 += msave1[i]-msave3[i];
			}

			if(accum2 < accum1)
			{
				i=1;
				if(accum3 < accum2) i=2;
			}
			else
			{
				i=0;
				if(accum3 < accum1) i=2;
			}

			switch(i)
			{
			default:
			case 0:
				LogData(msave1,0);
				break;
			case 1:
				LogData(msave2,0);
				break;
			case 2:
				LogData(msave3,0);
				break;
			}

			logging_state++;
			break;

		// back to sleep
		case 6:
			logging_state=0;

			break;

	}

}

struct rtc_calendar_time systime;

static uint16_t light;

static uint16_t temp,humid;
static int32_t Voc,Nox,O2,Voc2;
static reg_val_t S302, S402, S502, S602, S702, S802, NDIR;
static bool activate_heater;

static uint8_t cnt,sstate;
static bool first,lsm_ok=false,lsm_read;
static bool freezer_enable = false;
static char textbuf[128];

void main_tasks(void);

void main_tasks(void)
{

SYSTEM_RunTasks();

Run_Demo();

if(usb_port_open())
Modbus(0);

// LoRa modbus slave
Modbus2(0);

// background eeprom eraser
if(erase_address)
BackgroundErase();

switch(sstate)
{
	default:
	case 0:
	sstate=0;
	break;

	case 1:

	light = SENSORS_getLight();
	sstate++;
	break;

	case 2:

	if(!SENSORS_getSHT45(&temp,&humid, activate_heater))
	{ temp=-1; humid=-1; }

	activate_heater=false;
	
	sstate++;
	break;

	case 3:
	if(lsm_ok)
	{
		lsm_read = LSM6_read();
	}
	sstate++;
	break;

	case 4:

	if(!SENSORS_getSGP41(&Voc, &Nox)) // 55ms
	{
		Voc=Nox=-1;
	}
	sstate++;
	break;

	case 5:
	
	
	#ifdef IGNORE_LMP
		
	ADC_StartConversion(ADC_CHAN_O2);
	sstate++;
	
	#else
	
	if(SENSORS_setLMPValue())
	{
		ADC_StartConversion(ADC_CHAN_O2);
		sstate++;
	}
	else
	{
		ADC_StartConversion(ADC_CHAN_302);
		sstate+=2;
	}
	
	#endif
	
	break;

	case 6:
	if(ADC_WaitConversion())
	{
		O2 = ADC_ReadConversion();
		ADC_StartConversion(ADC_CHAN_302);
		sstate++;
	}
	break;

	case 7:
	if(ADC_WaitConversion())
	{
		S302 = ADC_ReadConversion();
		ADC_StartConversion(ADC_CHAN_402);
		sstate++;
	}
	break;

	case 8:
	if(ADC_WaitConversion())
	{
		S402 = ADC_ReadConversion();
		ADC_StartConversion(ADC_CHAN_502);
		sstate++;
	}
	break;

	case 9:
	if(ADC_WaitConversion())
	{
		S502 = ADC_ReadConversion();
		ADC_StartConversion(ADC_CHAN_602);
		sstate++;
	}

	break;

	case 10:
	if(ADC_WaitConversion())
	{
		S602 = ADC_ReadConversion();
		ADC_StartConversion(ADC_CHAN_702);
		sstate++;
	}
	break;

	case 11:
	if(ADC_WaitConversion())
	{
		S702 = ADC_ReadConversion();
		ADC_StartConversion(ADC_CHAN_802);
		sstate++;
	}
	break;

	case 12:
	if(ADC_WaitConversion())
	{
		S802 = ADC_ReadConversion();
		ADC_StartConversion(ADC_CHAN_NDIR);
		sstate++;
	}

	break;

	case 13:
	if(ADC_WaitConversion())
	{
		NDIR = ADC_ReadConversion();
		sstate++;
	}
	break;

	case 14:

	if(!SENSORS_getSGP40(&Voc2))
	{
		Voc2=-1;
	}

	sstate++;
	break;

	case 15:


	// copy registers
	measurements[0] = (uint16_t)light;
	measurements[1] = (uint16_t)temp;
	measurements[2] = (uint16_t)humid;
	measurements[3] = (uint16_t)Voc;
	measurements[4] = (uint16_t)Nox;
	measurements[5] = (uint16_t)O2;
	measurements[6] = (uint16_t)NDIR;
	measurements[7] = (uint16_t)S302;
	measurements[8] = (uint16_t)S402;
	measurements[9] = (uint16_t)S502;
	measurements[10] = (uint16_t)S602;
	measurements[11] = (uint16_t)S702;
	measurements[12] = (uint16_t)S802;
	measurements[13] = (uint16_t)Voc2;

	//				sstate=0;
	sstate=1;	// **testing** keep sampling
	break;
}

if(cnt != cb_counter)
{
	cnt = cb_counter;

	rtc_get_time(&systime);

	// work out the entry number based on the current time and the logging interval 0..1439
	entry = systime.hour;
	entry *= 60;
	entry += systime.minute;

	if(logging_state || (entry==start_entry))
	{
		//	SENSORS_3V3_SENS(1);
		//	SENSORS_5V_CTRL(1);

		if(!sstate)
		sstate=1;

		LED_Toggle(LED0);
	}
	else
	{
		if(!sstate)
		{
			//	SENSORS_3V3_SENS(0);
			//	SENSORS_5V_CTRL(0);
		}
		LED_Off(LED0);
	}

	#ifdef DUMP_SENSOR

	sprintf(tbuf,"L:%2d T:%2d H:%2d", (int)light,(int)temp, (int)humid);
	ssd1306_setpos(0,0);
	ssd1306_string_font6x8(tbuf);


	if(lsm_ok && lsm_read && !first)
	{

		sprintf(tbuf,"%2d:%2d:%2d %2d:%2d:%2d", (int)LSM6_da[0], (int)LSM6_da[1], (int)LSM6_da[2], (int)LSM6_dg[0], (int)LSM6_dg[1], (int)LSM6_dg[2] );
		ssd1306_setpos(0,1);
		ssd1306_string_font6x8(tbuf);
	}

	sprintf(tbuf,"Voc:%-4ld Nox:%-4ld", (long)Voc, (long)Nox);
	ssd1306_setpos(0,2);
	ssd1306_string_font6x8(tbuf);
	sprintf(tbuf,"O2:%4lX NDIR:%4X", (long)O2, (int)NDIR);
	ssd1306_setpos(0,3);
	ssd1306_string_font6x8(tbuf);
	sprintf(tbuf,"302:%4X 402:%4X", (int)S302,(int)S402);
	ssd1306_setpos(0,4);
	ssd1306_string_font6x8(tbuf);
	sprintf(tbuf,"502:%4X 602:%4X", (int)S502,(int)S602);
	ssd1306_setpos(0,5);
	ssd1306_string_font6x8(tbuf);
	sprintf(tbuf,"702:%4X 802:%4X", (int)S702,(int)S802);
	ssd1306_setpos(0,6);
	ssd1306_string_font6x8(tbuf);

	sprintf(tbuf,"Voc2:%-4ld", (long)Voc2);
	ssd1306_setpos(0,7);
	ssd1306_string_font6x8(tbuf);

	#endif

	if(sleep_period)
	entry /= sleep_period;

	if((entry != last_entry) || first)
	{
		if(!logging_state && !first && !erase_address)
		{
			// start up process
			logging_state=1;
			
		}

		last_entry = entry;
		first=false;
	}

	if(!erase_address)
	{
		
		
		if(update_sleep_period)
		{
			WriteSleepPeriod();
			update_sleep_period=false;
		}

		//printf("%02d:%02d:%02d %d/%d/%d\r\n",  systime.hour,systime.minute,systime.second, systime.day,systime.month, systime.year);
		//sprintf(tbuf,"%02d:%02d:%02d %d/%d/%d\r\n",  systime.hour,systime.minute,systime.second, systime.day,systime.month, systime.year);

		
		RunLogger();
	}
}
	
	
}

int main ( void )
{
	
	irq_initialize_vectors();

	system_init();
	delay_init();
	LED_On(LED0);
	i2c1_init();
	i2c2_init();

	// Initialize the sleep manager
	sleepmgr_init();

	cpu_irq_enable();

	rtc_init();

	SENSORS_init();
	servo_init();

	RN4871_init();

	// Start USB stack to authorize VBus monitoring
	udc_start();

	//lsm_ok=LSM6_init(device_DSO, sa0_auto);
	//if(lsm_ok)
	//	LSM6_enableDefault();

#if defined (ENABLE_LCD)
	LCD_Initialize();
#endif

	sio2host_init();

	// Permanent address for the device is set in miwi_config.h and loaded in global variable myLongAddress
	// Define SYMBOL MACRO to populate myLongAddress from EDBG_EUI or from MODULE_EUI
#if (EDBG_EUI_READ == 1 || MODULE_EUI_READ == 1)
	ReadMacAddress();
#endif

	SystemTimerInit();

	//RN4871_Setup("Cadlink");

    // Demo Start Message
    DemoOutput_Greeting();

	nvm_init(INT_FLASH);

#if defined(ENABLE_NETWORK_FREEZER)



	PDS_Init();

    demo_output_freezer_options();
    // User Selection to commission a network or use Freezer
   freezer_enable = freezer_feature();
#endif

    // Commission the network
    Initialize_Demo(freezer_enable);

	/* Initialize demo application */
    Stack_Init();

	Modbus_initialise(1);
	Modbus2_initialise(3);

	I2CEEPROM(CAT24_ADDR1, EEPROM_DEVICE_CAT24C512);

	cnt=cb_counter;
	first=true;
	sstate=1;
	sleep_period=1;	// minutes between logs .. reload this from NV memory
	erase_address=0;
	logging_state=0;
	activate_heater=false;
	
	if(!ReadSleepPeriod())
		update_sleep_period=true;
	

	SENSORS_3V3_SENS(1);
	SENSORS_5V_CTRL(1);

	LED_Off(LED0);

	rtc_get_time(&systime);
	
	start_entry = systime.hour;
	start_entry *= 60;
	start_entry += systime.minute;

	//GPS_init();
	
    while(1)
    {
		//GPS_poll();
		
		main_tasks();
		
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


#define LOGSINGLESIZE (LOGCHANNELS*2u)
#define LOGENTRYSIZE (LOGSINGLESIZE*LOGDUPLICATES)

/// EEPROM day structure (1 minute logs)

#define LOGSPERDAYM 1440u

typedef struct {

	uint8_t day,month,yr;		// timestamp
	uint8_t period;				// period time (mins)

	uint16_t data[LOGSPERDAYM*LOGCHANNELS*LOGDUPLICATES];	// data logged at 1minute intervals (max)

} DAYRECORDM;

/// Days stored in (EEPROM = 2x65536 bytes)
#define NUMDAYSM 3u

/// EEPROM day structure (1 hr logs)

#define LOGSPERDAYH 24u

typedef struct {

	uint8_t day,month,yr;		// timestamp
	uint8_t period;			 	// period time (hrs |bit7)

	uint16_t data[LOGSPERDAYH*LOGCHANNELS*LOGDUPLICATES];	// data logged at 1hr intervals

} DAYRECORDH;

/// Days stored in EEPROM (EEPROM = 2x65536 bytes)

#define NUMDAYSH 193u

static uint16_t NUMDAYS(void)
{
	if(sleep_period < 60)
		return NUMDAYSM;
	else
		return NUMDAYSH;
}

static uint16_t DAYRECORD(void)
{
	if(sleep_period < 60)
		return sizeof(DAYRECORDM);
	else
		return sizeof(DAYRECORDH);
}

// compute address of a particular day no 0..n-1
static XEE_ADDR compute_addr(uint16_t day_no)
{
	XEE_ADDR addr;

	addr = day_no;
	addr *= DAYRECORD();

	return addr+2;
}

static void EraseLogs(void)
{

	uint16_t i;
	XEE_ADDR addr;
	uint8_t array[4];

	for(i=0;i<NUMDAYS();i++)
	{
		addr = compute_addr(i);

		// Clear the day's date structure
		array[0]=255;
		array[1]=255;
		array[2]=255;
		array[3]=255;

		XEEWriteArray(addr, array, 4u);
	}

	// start the background wipe
	erase_address=2;
}


bool	Write_register(uint16_t address,uint16_t data)
{
	switch(address)
	{
		default: return false;

		case 0: clockset.second=data; break;
		case 1: clockset.minute=data; break;
		case 2: clockset.hour=data; break;
		case 3: clockset.day=data; break;
		case 4: clockset.month=data; break;
		case 5: clockset.year=data; break;
		case 6: if(data) load_clk=true; break;
		case 7: if(data < NUMDAYS()) day_index=data; break;

		case 8: sleep_period = data; update_sleep_period=true; break;

		case 9: if(data==5150u) { EraseLogs(); WriteSleepPeriod(); } break;

		case 10: if(data==0u || data==1u) { servo=(uint8_t)data; UpdateServo(); } break;
			
		case 11: if(data==1u) activate_heater=true; break;
			
		case 12: if(data==0u || data==1u) { heaters=(uint8_t)data; UpdateHeaters(); } break;
	}

	return true;
}


uint16_t Read_Holding_Register(uint16_t address)
{
	uint8_t array[2];
	XEE_ADDR addr;
	uint16_t t;

	switch(address)
	{
		default:
		if((address >= 1000u) && (address < 1000u+NUMDAYS()*2))
		{
			address-=1000u;

			// day offset structure

			addr = compute_addr(address / 2);

			XEEReadArray(addr + (address & 1)*2, array, 2u);

			t=array[0];
			t<<=8;
			t|=array[1];

			return t;
		}
		else
		return 0;

		case 0: return clockset.second;
		case 1: return clockset.minute;
		case 2: return clockset.hour;
		case 3: return clockset.day;
		case 4: return clockset.month;
		case 5: return clockset.year;
		case 6: return 0;
		case 7: return day_index;
		case 8: return sleep_period;
		case 9: return 0;
		case 10: return servo;
		case 11: return activate_heater;
		case 12: return heaters;
	}

}

uint16_t Read_Input_Register(uint16_t address)
{
	XEE_ADDR addr;
	uint16_t t;
	uint8_t array[2];

	switch(address)
	{
		default:

		if(address >= 1000u && address < 1000u+(DAYRECORD()/2))
		{
			address-=1000u;

			// compute address within current day
			addr = compute_addr(day_index);

			// add offset
			addr += (address<<1);

			XEEReadArray(addr, array, 2u);

			t=array[0];
			t<<=8;
			t|=array[1];

			return t;
		}
		else
		{
			return 0;
		}

		case 0:
			rtc_get_time(&systime);
			return systime.second;
		case 1: return systime.minute;
		case 2: return systime.hour;
		case 3: return systime.day;
		case 4: return systime.month;
		case 5: return systime.year;

		case 6:  return measurements[0];
		case 7:  return measurements[1];
		case 8:  return measurements[2];
		case 9:  return measurements[3];
		case 10: return measurements[4];
		case 11: return measurements[5];
		case 12: return measurements[6];
		case 13: return measurements[7];
		case 14: return measurements[8];
		case 15: return measurements[9];
		case 16: return measurements[10];
		case 17: return measurements[11];
		case 18: return measurements[12];
		case 19: return measurements[13];

		case 20: return heaters;

		case 21: return gps_state;
		case 22: return (U16)(gps_lat>>16);
		case 23: return (U16)gps_lat;
		case 24: return (U16)(gps_lon>>16);
		case 25: return (U16)gps_lon;
	}

}

///
/// Log adc measurements in EEPROM
///
/// offset = 0... LOGDUPLICATES-1
///
bool LogData(uint16_t *measurement, uint8_t _offset)
{
	uint8_t d,m,y,p,pm,first_rec;
	XEE_ADDR _addr;
	uint32_t old_dateno,current_dateno;
	uint8_t _array[LOGSINGLESIZE];
	uint16_t i,old_i;
	char str[32];

	first_rec=1;
	old_i=0;

	if(sleep_period < 60)
		pm = (uint8_t)sleep_period;
	else
	{
		pm = (uint8_t)(sleep_period / 60);
		pm |= 128;
	}

	// Locate the day in EEPROM

	for(i=0;i<NUMDAYS();i++)
	{
		_addr = compute_addr(i);

		XEEReadArray(_addr, _array, 4u);

		d=_array[0];
		m=_array[1];
		y=_array[2];
		p=_array[3];

		// exact match .. exit
		if(d==systime.day && m==systime.month && y==(systime.year-2000) && pm==p)
			break;

		if(d!=0xFF && m!=0xFF && y < 50u)
		{
			current_dateno=y;
			current_dateno*=12;
			current_dateno+=m;
			current_dateno*=32;
			current_dateno+=d;
		}
		else
		{
			current_dateno=0xFFFFu;	// invalid record
		}


		// record entry if we dont have one yet
		if(first_rec)
		{
			old_dateno=current_dateno;

			first_rec=0;
		}

		if(old_dateno!=0xFFFFu)
		{
			if(current_dateno==0xFFFFu || (current_dateno < old_dateno))
			{
				old_dateno=current_dateno;
				old_i=i;
			}
		}


	}

	// use oldest/invalid entry if day not found
	if(i==NUMDAYS())
	{
		i=old_i;

		_addr = compute_addr(i);

		// Clear the structure
		_array[0]=systime.day;
		_array[1]=systime.month;
		_array[2]=systime.year-2000;
		_array[3]=pm;

		XEEWriteArray(_addr, _array, 4u);

	}

	// day offset
	_addr = compute_addr(i);

	_addr += 4; // skip header

	// go to time entry
	_addr += entry*LOGENTRYSIZE;

	// select the correct duplicate
	_addr += (_offset&(LOGDUPLICATES-1))*LOGSINGLESIZE;

	// write with verification (5 retries)

	for(y=0;y<6u;y++)
	{
		XEEWriteArray(_addr, (uint8_t *)measurement, LOGSINGLESIZE);

		// Verify
		XEEReadArray(_addr, _array, LOGSINGLESIZE);

		for(i=0;i!=LOGSINGLESIZE;i++)
		{
			if(_array[i] != ((uint8_t *)measurement)[i])
				break;
		}

		if(i==LOGSINGLESIZE)
			break;
	}

	//if(y==6) DIAGNOSTIC++;

	return y!=6u;
}


