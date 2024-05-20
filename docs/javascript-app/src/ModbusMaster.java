/*
 *
 *  Modbus master class
 *
 */

import java.io.*;
import java.util.*;
import gnu.io.*;    			// rxtx communcation's api plug-in
import java.net.* ;

public class ModbusMaster {

  // Serial Port
  private SerialPort sPort;
  private boolean open;
  private SerialParameters parameters;
  private OutputStream streamo;
  private InputStream streami;
  private CommPortIdentifier portId;
  private byte buffer[];
  private byte packet[];
  private byte preamble[];
  private final int timeout=1000;			// how long to wait for a response ms
  private boolean retries;
  // Modbus stuff
  int uchCRCHi,uchCRCLo;
  byte point_buffer[];
  int point_data;
  private int sleepTime = 10;  	// polling loop time for incoming data
  int device;

  public ModbusMaster(SerialParameters parameters,int device)
  {
    open=false;
    retries=false;
    this.parameters = parameters;
	this.device = device;

    buffer = new byte[256];
    packet = new byte[16];
    preamble = new byte[3+8];

  }

   public void setDevice(int device)
   {
   	this.device = device;
   }

   /**
   Attempts to open a serial connection and streams using the parameters
   in the SerialParameters object. If it is unsuccesfull at any step it
   returns the port to a closed state, throws a
   <code>SerialConnectionException</code>, and returns.

   Gives a timeout of 30 seconds on the portOpen to allow other applications
   to reliquish the port if have it open and no longer need it.
   */

  public void openConnection()
    throws Exception
  {




    	// Obtain a CommPortIdentifier object for the port you want to open.
    	try
    	{
    	    portId = CommPortIdentifier.getPortIdentifier(parameters.getPortName());
    	} catch (NoSuchPortException e)
    	{
    	    throw new SerialConnectionException(e.getMessage());
    	}

    	// Open the port represented by the CommPortIdentifier object. Give
    	// the open call a relatively long timeout of 30 seconds to allow
    	// a different application to reliquish the port if the user
    	// wants to.
    	try
    	{
    	    sPort = (SerialPort)portId.open("ModbusMaster", 10000);
    	}
    	catch (PortInUseException e)
    	{
    	    throw new SerialConnectionException(e.getMessage());
    	}

	    // Set the parameters of the connection. If they won't set, close the
	    // port before throwing an exception.

	    try
	    {
	        setConnectionParameters();
	    }
	    catch (SerialConnectionException e)
	    {
	        sPort.close();
	        throw e;
	    }


    	// Open the input and output streams for the connection. If they won't
    	// open, close the port before throwing an exception.
    	try
    	{
    	    streamo = sPort.getOutputStream();
    	    streami = sPort.getInputStream();
    	} catch (IOException e)
    	{
    	    sPort.close();
    	    throw new SerialConnectionException("Error opening i/o streams");
    	}

        sPort.setDTR(true);
        sPort.setRTS(true);

	//	System.out.println("openConnection: OK");

    open = true;

    }


    /**
    Sets the connection parameters to the setting in the parameters object.
    If set fails return the parameters object to origional settings and
    throw exception.
    */
    public void setConnectionParameters() throws SerialConnectionException {


    		// Save state of parameters before trying a set.
    		int oldBaudRate = sPort.getBaudRate();
    		int oldDatabits = sPort.getDataBits();
    		int oldStopbits = sPort.getStopBits();
    		int oldParity   = sPort.getParity();
    		int oldFlowControl = sPort.getFlowControlMode();

	//		System.out.println("Parameters="+parameters.getPortName()+","+parameters.getBaudRate()+","+parameters.getDatabits()+","+parameters.getParity()+","+parameters.getStopbits());

    		// Set connection parameters, if set fails return parameters object
    		// to original state.
    		try
    		{
        		sPort.setSerialPortParams(parameters.getBaudRate(),
                      parameters.getDatabits(),
                      parameters.getStopbits(),
                      parameters.getParity());

        		//sPort.enableReceiveThreshold(256);
    		}
    		catch (UnsupportedCommOperationException e)
    		{
        		parameters.setBaudRate(oldBaudRate);
        		parameters.setDatabits(oldDatabits);
        		parameters.setStopbits(oldStopbits);
        		parameters.setParity(oldParity);
        		throw new SerialConnectionException("Unsupported parameter: "+e);
    		}

    		// Set flow control.
    		try
    		{
        		sPort.setFlowControlMode(parameters.getFlowControlIn()
                       | parameters.getFlowControlOut());
     		} catch (UnsupportedCommOperationException e)
     		{
        		throw new SerialConnectionException("Unsupported flow control");
      		}


    }

    /**
    Close the port and clean up associated elements.
    */
    public void closeConnection() {
    // If port is alread closed just return.
    if (!open) {
        return;
    }

    	// Check to make sure sPort has reference to avoid a NPE.
    	if (sPort != null)
   	 	{
        	try
        	{
        		// close the i/o streams.
            	streamo.close();
            	streami.close();
        	}
        	catch (IOException e)
        	{
				System.out.println("Unable to close port - "+e);
        	}

        	// Close the port.
        	sPort.close();

	 	}

    open = false;
    }

    /**
    Reports the open status of the port.
    @return true if port is open, false if port is closed.
    */
    public boolean isOpen() {
        return open;
    }

  public void initCRC16()
  {
    uchCRCHi = 0xff ;   // high CRC byte initialized
    uchCRCLo = 0xff ;   // low CRC byte initialized
  } // End initCRC16()

  // compute the CRC16 for a given packet
  // returns in the globals uchCRCHi,uchCRCLo
  public void CRC16(byte p[],int offset,int length)
  {
    // Table of CRC values for high-order byte

    int auchCRCHi[] = {
0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1,
0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1,
0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40,
0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1,
0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40,
0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40,
0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1,
0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
0x80, 0x41, 0x00, 0xC1, 0x81, 0x40
} ;

    // Table of CRC values for low-order byte

    int auchCRCLo[] = {
0x00, 0xC0, 0xC1, 0x01, 0xC3, 0x03, 0x02, 0xC2, 0xC6, 0x06,
0x07, 0xC7, 0x05, 0xC5, 0xC4, 0x04, 0xCC, 0x0C, 0x0D, 0xCD,
0x0F, 0xCF, 0xCE, 0x0E, 0x0A, 0xCA, 0xCB, 0x0B, 0xC9, 0x09,
0x08, 0xC8, 0xD8, 0x18, 0x19, 0xD9, 0x1B, 0xDB, 0xDA, 0x1A,
0x1E, 0xDE, 0xDF, 0x1F, 0xDD, 0x1D, 0x1C, 0xDC, 0x14, 0xD4,
0xD5, 0x15, 0xD7, 0x17, 0x16, 0xD6, 0xD2, 0x12, 0x13, 0xD3,
0x11, 0xD1, 0xD0, 0x10, 0xF0, 0x30, 0x31, 0xF1, 0x33, 0xF3,
0xF2, 0x32, 0x36, 0xF6, 0xF7, 0x37, 0xF5, 0x35, 0x34, 0xF4,
0x3C, 0xFC, 0xFD, 0x3D, 0xFF, 0x3F, 0x3E, 0xFE, 0xFA, 0x3A,
0x3B, 0xFB, 0x39, 0xF9, 0xF8, 0x38, 0x28, 0xE8, 0xE9, 0x29,
0xEB, 0x2B, 0x2A, 0xEA, 0xEE, 0x2E, 0x2F, 0xEF, 0x2D, 0xED,
0xEC, 0x2C, 0xE4, 0x24, 0x25, 0xE5, 0x27, 0xE7, 0xE6, 0x26,
0x22, 0xE2, 0xE3, 0x23, 0xE1, 0x21, 0x20, 0xE0, 0xA0, 0x60,
0x61, 0xA1, 0x63, 0xA3, 0xA2, 0x62, 0x66, 0xA6, 0xA7, 0x67,
0xA5, 0x65, 0x64, 0xA4, 0x6C, 0xAC, 0xAD, 0x6D, 0xAF, 0x6F,
0x6E, 0xAE, 0xAA, 0x6A, 0x6B, 0xAB, 0x69, 0xA9, 0xA8, 0x68,
0x78, 0xB8, 0xB9, 0x79, 0xBB, 0x7B, 0x7A, 0xBA, 0xBE, 0x7E,
0x7F, 0xBF, 0x7D, 0xBD, 0xBC, 0x7C, 0xB4, 0x74, 0x75, 0xB5,
0x77, 0xB7, 0xB6, 0x76, 0x72, 0xB2, 0xB3, 0x73, 0xB1, 0x71,
0x70, 0xB0, 0x50, 0x90, 0x91, 0x51, 0x93, 0x53, 0x52, 0x92,
0x96, 0x56, 0x57, 0x97, 0x55, 0x95, 0x94, 0x54, 0x9C, 0x5C,
0x5D, 0x9D, 0x5F, 0x9F, 0x9E, 0x5E, 0x5A, 0x9A, 0x9B, 0x5B,
0x99, 0x59, 0x58, 0x98, 0x88, 0x48, 0x49, 0x89, 0x4B, 0x8B,
0x8A, 0x4A, 0x4E, 0x8E, 0x8F, 0x4F, 0x8D, 0x4D, 0x4C, 0x8C,
0x44, 0x84, 0x85, 0x45, 0x87, 0x47, 0x46, 0x86, 0x82, 0x42,
0x43, 0x83, 0x41, 0x81, 0x80, 0x40
} ;
    int index;
    int uIndex ;    // will index into CRC lookup

    for(index=0;index<length;index++)
    {


     uIndex = uchCRCHi ^ ((int)p[index+offset] & 0xff); // calculate the CRC

     uchCRCHi = uchCRCLo ^ auchCRCHi[uIndex] ;
     uchCRCLo = auchCRCLo[uIndex] ;
    }

  } // End CRC16

  // Wait for data to be present on the input stream
  public boolean waitData(int bytes,long _timeout)
  {
    int gotbytes=0;

	//_timeout*=2;

    while((gotbytes < bytes) && (_timeout > 0))
    {

     	try
     	{
	        gotbytes = streami.available();

	     //   System.out.println("Gotbytes="+gotbytes+" wait4="+bytes);

	     }
     	catch(IOException e)
     	{
            System.out.println("Can't read bytes available?! "+e);

			gotbytes = 0;

			return false;

     	}

     	if(gotbytes < bytes)
     	{
        	try
            { // Snoozing a bit.
                Thread.sleep(sleepTime);
            }
            catch (Exception e)
            {
                System.out.println("Couldn't sleep!");
            }
        	_timeout -= sleepTime;
     	}
    }

	if(gotbytes < bytes)
	{
			System.out.println("Expected "+bytes+" bytes but got only "+gotbytes);
	}

    return (gotbytes >= bytes);

  } // End waitData

  // read modbus 3x or 4x registers into array
  public synchronized boolean ModbusRead(int modbus_addr,int num_points,int data[])
  {

    int i,j,retry,num_bytes,garbage,act_points,offset;
    byte fn;

     //		sPort.setRTS(true);

    if(modbus_addr < 30001)
    {
        System.out.println("Modbus read address not supported!");
        return false;
    }
    else if(modbus_addr < 40000)
    {   fn = 4; modbus_addr -= 30001; }
    else if(modbus_addr < 50000)
    {   fn = 3; modbus_addr -= 40001; }
    else
    {
        System.out.println("Modbus read address not supported!");
        return false;
    }

    offset=0;

    while (num_points > 0)
    {

       // break request into requests of maximum size

       if(num_points > 64)
	   {
           act_points = 64;

		//   System.out.println("Points to go="+num_points);
		}
       else
           act_points = num_points;

        for(retry=1;retry>0;retry--)
        {



                try
                {
                garbage=streami.available();
                if(garbage > 0)
                        streami.skip(garbage);
                }
                catch(IOException e)
                {
					System.out.println("Can't read bytes available?! "+e);

					return false;
				}


        System.out.println("Read addr="+fn+"/"+(modbus_addr+offset)+" points="+act_points+" device="+device);


        // frame the modbus request packet
        packet[0]=(byte)device;
        packet[1]=fn;
        packet[2]=(byte)((modbus_addr+offset) >> 8);
        packet[3]=(byte)((modbus_addr+offset) & 0xFF);
        packet[4]=(byte)(act_points >> 8);
        packet[5]=(byte)(act_points & 0xFF);

         initCRC16();
         CRC16(packet,0,6);
         packet[6]=(byte)uchCRCHi;
         packet[7]=(byte)uchCRCLo;

        try
        {

			 preamble[0]=(byte)255;
			 preamble[1]=(byte)255;
			 preamble[2]=(byte)255;
			 streamo.write(preamble,0,3);
             streamo.write(packet,0,8);

            streamo.flush();

/*
       		try
            { // Snoozing a bit.
                Thread.sleep(11);
            }
            catch (Exception e)
            {
                System.out.println("Couldn't sleep!");
            }

     		sPort.setRTS(false);
*/

            if(retry < 1)
            {
			 retries=true;
             System.out.print("Read retry ");
             System.out.println(1-retry);
            }

            packet[0]=0;
            while(true==waitData(1,timeout))
            {

             // read slave address
             streami.read(packet,0,1);
             if(packet[0] == device)
               break;

			 System.out.println("DEV "+(packet[0] & 255));
            }

            if(packet[0] == device)
            {
              if(true==waitData(2,timeout))
              {
               // read function code & byte count
               streami.read(packet,1,2);
               if(packet[1] == fn)
               {

                // compute effective byte count
                num_bytes=packet[2] & 0xFF;
                if(num_bytes==0)
                    num_bytes=256;

              //  System.out.println(num_bytes+"=bytes in body");


                // wait for data + CRC if enabled
                i=num_bytes;
                i+=2;

                if(true==waitData(i,timeout+i*10))
                {
                 // read data
                 streami.read(buffer,0,num_bytes);

                 // read crc
                 streami.read(packet,3,2);

                 // check CRC
                 initCRC16();
                 CRC16(packet,0,3);
                 CRC16(buffer,0,num_bytes);
                 CRC16(packet,3,2);


                 if(uchCRCHi==0 && uchCRCLo==0)
                 {
                    // copy data into output array
                    for(i=0,j=0;i<act_points && j<num_bytes;i++,j+=2)
                    {
                        data[i+offset] = buffer[j] & 0xFF;
                        data[i+offset] <<= 8;
                        data[i+offset] += buffer[j+1] & 0XFF;

                        //System.out.println("Addr "+fn+"/"+(modbus_addr+offset+i)+"="+data[i]);
                    }

                    // break out of retry loop
                    break;

                 }
                 else
                    System.out.println("CRC failed");

                } // wait for DATA+CRC
                else
                    System.out.println("Timeout in body");

               } // check FN code
               else
               {
				   if((packet[1] & 128)!=0 && ((packet[1] & 127)==fn))
				   {
					System.out.println("Modbus error "+(packet[2] & 255));
				   }
				   else
	                System.out.println("Bad function code "+(packet[1] & 255)+" "+(packet[2] & 255));
				}
              } // waiting for header
              else
                System.out.println("Timeout in header FN");

            } // wait for header
            else
                System.out.println("Timeout on header UNIT");

        } // general exception handling
        catch(IOException e)
        {
            System.out.println("I/O exception in read algorithm "+e);

				retries=true;
	            return false;

        }


        try
        { // Snoozing a bit.
                Thread.sleep(sleepTime);
        }
        catch (Exception e)
        {
                System.out.println("Couldn't sleep?!");
        }
     } // of retry loop


     // exit if failure
     if(retry == 0)
        return false;

     // advance to next group of points
     offset += act_points;
     num_points -= act_points;

    } // of while loop

	retries=false;

    return true;

  } // End ModbusRead


  // write single modbus 4x registers
  public synchronized boolean ModbusWriteSingle(int modbus_addr,int data)
  {

    int retry,length,i;
    byte fn;

    if(modbus_addr < 50000 && modbus_addr > 40000)
    {   fn = 6; modbus_addr -= 40001; }
    else
    {
        System.out.println("Modbus write address not supported!");
        return false;
    }

    for(retry=1;retry>0;retry--)
    {

    	System.out.println("Write addr "+modbus_addr+"="+data);

        // frame the modbus request packet
        packet[0]=(byte)device;
        packet[1]=fn;
        packet[2]=(byte)(modbus_addr >> 8);
        packet[3]=(byte)(modbus_addr & 0xFF);
        packet[4]=(byte)(data >> 8);
        packet[5]=(byte)(data & 0xFF);

        this.initCRC16();
        this.CRC16(packet,0,6);

        packet[6]=(byte)uchCRCHi;
        packet[7]=(byte)uchCRCLo;

        if(retry < 1)
        {
         System.out.println("Write retry "+(1-retry));
		 retries=true;
        }

        try
        {


				 preamble[0]=(byte)255;
			 	streamo.write(preamble,0,1);
            	streamo.write(packet,0,8);

            streamo.flush();

            packet[0]=0; i=0;
            while(true==waitData(1,timeout))
            {

             // read slave address
             streami.read(packet,0,1);

				 //System.out.println("DEV "+(packet[0] & 255));
             if(packet[0] == device)
                break;
            }


            if(packet[0] == device)
            {
				i=5;

					i+=2;

              if(true==waitData(i,timeout))
              {
               // read function code, address, points & crc if enabled
               streami.read(packet,1,i);

               if(packet[1] == fn)
               {


                	// check CRC
                	initCRC16();
                	CRC16(packet,0,8);


                if(uchCRCHi==0 && uchCRCLo==0)
                {

					//System.out.println("DATA "+(packet[2]&255)+" "+(packet[3]&255)+" "+(packet[4]&255)+" "+(packet[5]&255));

					//System.out.println("OK");
					retries=false;

                    return true;
				}
                else
                    System.out.println("CRC failed");
                } // check FN code
                else
                 System.out.println("Bad function code "+(packet[1] & 255));
               } // waiting for header
               else
                System.out.println("Timeout waiting for header");

             } // check unit addr
             else
                System.out.println("Timeout on header UNIT");

        } // general exception handling
        catch(IOException e)
        {
            System.out.println("I/O exception in write algorithm "+e);


				retries=true;
				return false;

        }

        if(sleepTime > 0)
        {

         try
         { // Snoozing a bit.
                Thread.sleep(sleepTime);
         }
         catch (Exception e)
         {
                System.out.println("Couldn't sleep?!");
                return false;
         }
        }

    } // of retry loop


	retries=true;

    return false;

  } // End ModbusWrite

  // read modbus 3x or 4x registers into array
  public synchronized boolean ModbusReadFn(int fnn, int modbus_addr,int num_points,int data[])
  {

    int i,j,retry,num_bytes,garbage,act_points,offset;
    byte fn;

	fn = (byte)fnn;

    offset=0;

    while (num_points > 0)
    {

       // break request into requests of maximum size

       if(num_points > 96)
	   {
           act_points = 96;

		//   System.out.println("Points to go="+num_points);
		}
       else
           act_points = num_points;

        for(retry=3;retry>0;retry--)
        {



                try
                {
                garbage=streami.available();
                if(garbage > 0)
                        streami.skip(garbage);
                }
                catch(IOException e)
                {
					System.out.println("Can't read bytes available?! "+e);

					return false;
				}


        //System.out.println("Read dev="+device+" addr="+fn+"/"+(modbus_addr+offset)+" points="+act_points);


        // frame the modbus request packet
        packet[0]=(byte)device;
        packet[1]=fn;
        packet[2]=(byte)((modbus_addr+offset) >> 8);
        packet[3]=(byte)((modbus_addr+offset) & 0xFF);
        packet[4]=(byte)(act_points >> 8);
        packet[5]=(byte)(act_points & 0xFF);

         initCRC16();
         CRC16(packet,0,6);
         packet[6]=(byte)uchCRCHi;
         packet[7]=(byte)uchCRCLo;

        try
        {

        try
        { // Snoozing a bit.
                Thread.sleep(10);
        }
        catch (Exception e)
        {
                System.out.println("Couldn't sleep?!");
        }

			 preamble[0]=(byte)255;
			 preamble[1]=(byte)255;
			 preamble[2]=(byte)255;
			 for(i=0;i!=8;i++)
			 	preamble[3+i]=packet[i];

			 streamo.write(preamble,0,3+8);

            streamo.flush();

            if(retry < 3)
            {
			 retries=true;
             System.out.print("Read retry ");
             System.out.println(3-retry);
            }

            packet[0]=0;
            while(true==waitData(1,timeout))
            {

             // read slave address
             streami.read(packet,0,1);
             if(packet[0] == device)
               break;

		//	 System.out.println("DEV "+(packet[0] & 255));
            }

            if(packet[0] == device)
            {
              if(true==waitData(2,timeout))
              {
               // read function code & byte count
               streami.read(packet,1,2);
               if(packet[1] == fn)
               {

                // compute effective byte count
                num_bytes=packet[2] & 0xFF;
                if(num_bytes==0)
                    num_bytes=256;

             //   System.out.println(num_bytes+"=bytes in body");


                // wait for data + CRC if enabled
                i=num_bytes;
                i+=2;

                if(true==waitData(i,timeout+i*10))
                {
                 // read data
                 streami.read(buffer,0,num_bytes);

                 // read crc
                 streami.read(packet,3,2);

                 // check CRC
                 initCRC16();
                 CRC16(packet,0,3);
                 CRC16(buffer,0,num_bytes);
                 CRC16(packet,3,2);


                 if(uchCRCHi==0 && uchCRCLo==0)
                 {
                    // copy data into output array
                    for(i=0,j=0;i<act_points && j<num_bytes;i++,j+=2)
                    {
                        data[i+offset] = buffer[j] & 0xFF;
                        data[i+offset] <<= 8;
                        data[i+offset] += buffer[j+1] & 0XFF;

                        //System.out.println("Addr "+fn+"/"+(modbus_addr+offset+i)+"="+data[i]);
                    }

                    // break out of retry loop
                    break;

                 }
                 else
                    System.out.println("CRC failed");

                } // wait for DATA+CRC
                else
                    System.out.println("Timeout in body");

               } // check FN code
               else
                System.out.println("Bad function code "+(packet[1] & 255)+" "+(packet[2] & 255));
              } // waiting for header
              else
                System.out.println("Timeout in header FN");

            } // wait for header
            else
                System.out.println("Timeout on header UNIT");

        } // general exception handling
        catch(IOException e)
        {
            System.out.println("I/O exception in read algorithm "+e);

				retries=true;
	            return false;

        }


        try
        { // Snoozing a bit.
                Thread.sleep(sleepTime);
        }
        catch (Exception e)
        {
                System.out.println("Couldn't sleep?!");
        }
     } // of retry loop


     // exit if failure
     if(retry == 0)
        return false;

     // advance to next group of points
     offset += act_points;
     num_points -= act_points;

    } // of while loop



	retries=false;

    return true;

  } // End ModbusReadFn


} // End ModbusMaster.

