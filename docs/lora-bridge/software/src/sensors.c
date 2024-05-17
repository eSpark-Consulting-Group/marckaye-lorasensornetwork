
#include "sensors.h"
#include "miwi_api.h"


#include <sio2host.h>

void SENSORS_5V_CTRL(bool state)
{
	port_pin_set_output_level(PIN_5V_CNTL,state);
}

void SENSORS_MUXSEL(bool state)
{
	port_pin_set_output_level(PIN_MUXSEL,state);
}

bool SENSORS_init(void)
{
	
	struct port_config pin_conf;

	SENSORS_5V_CTRL(1);
	
	SENSORS_MUXSEL(1);
	
	port_get_config_defaults(&pin_conf);
	/* Configure the GPIO pin as outputs*/
	pin_conf.direction  = PORT_PIN_DIR_OUTPUT;
	port_pin_set_config(PIN_MUXSEL, &pin_conf);

	port_get_config_defaults(&pin_conf);
	/* Configure the GPIO pin as outputs*/
	pin_conf.direction  = PORT_PIN_DIR_OUTPUT;
	port_pin_set_config(PIN_5V_CNTL, &pin_conf);

	return true;

}
