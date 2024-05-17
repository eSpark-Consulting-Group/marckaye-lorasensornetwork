
#include <SparkFun_u-blox_GNSS_v3.h> 

SFE_UBLOX_GNSS myGNSS; // SFE_UBLOX_GNSS uses I2C. For Serial or SPI, see Example2 and Example3

uint8_t found=0;
int32_t latitude;
int32_t longitude;

void GPS_setup(void)
{
  
  //myGNSS.enableDebugging(); // Uncomment this line to enable helpful debug messages on Serial

	found=0;
	
}

void GPS_loop(void)
{
	if(found==0)
	{
		
		if (myGNSS.begin() == false) //Connect to the u-blox module using Wire port
			return;
			
		myGNSS.setI2COutput(COM_TYPE_UBX); //Set the I2C port to output UBX only (turn off NMEA noise)
		
		found=1;
	}
		
  // Request (poll) the position, velocity and time (PVT) information.
  // The module only responds when a new position is available. Default is once per second.
  // getPVT() returns true when new data is received.
  if (myGNSS.getPVT() == true)
  {
    latitude = myGNSS.getLatitude();
  
    longitude = myGNSS.getLongitude();

  }
}
