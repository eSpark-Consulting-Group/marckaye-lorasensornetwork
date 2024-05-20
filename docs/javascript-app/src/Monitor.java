

import java.util.*;
import java.io.*;
import javax.swing.*;
import javax.swing.text.*;
import java.awt.Toolkit;
import java.awt.FileDialog;
import java.text.SimpleDateFormat;
import java.text.FieldPosition;
import java.time.LocalDate;
import java.time.DateTimeException;

public class Monitor extends TimerTask
{
  private Cadlink parent;
  private boolean poll;
  private ModbusMaster mb;			// Modbus interface
  private LocalDate logstart,logend;
  private int logdev,logindex,logsize;
  private boolean demo;
  private int online[];
  private boolean sync_clock[];
  private boolean sleep_update,erase_logs_update,servo_update[],heater_trigger,heater_update[];
  private int sleep_period, servo_state[], heater_state[];
  private Progress bar;
  private int data[];
  private String logfile;

	public Monitor(Cadlink parent,	SerialParameters params)
	{
		demo=false;

		mb = new ModbusMaster(params, 2);

		data = new int[256];

		this.parent = parent;

		logdev=0;
		logsize=18;
		heater_trigger=false;

		online = new int[parent.stations];
		sync_clock = new boolean[parent.stations];
		servo_state = new int[parent.stations];
		servo_update = new boolean[parent.stations];
		heater_state = new int[parent.stations];
		heater_update = new boolean[parent.stations];

		for(int i=0;i<parent.stations;i++)
		{
			online[i]=0;
			sync_clock[i]=false;
			servo_update[i]=false;
			servo_state[i]=0;
			heater_update[i]=false;
			heater_state[i]=0;
		}

		sleep_update=false;
		erase_logs_update=false;

		poll=true;

	}

	private void dec_online(int dev)
	{
	//	System.out.println("DEC ONLINE "+dev);

	if(online[dev-1] > 0)
		online[dev-1] --;
	}

	private void inc_online(int dev)
	{
	//		System.out.println("INC ONLINE "+dev);

	if(online[dev-1] < 8)
		online[dev-1]+=2;
	}

	public boolean is_online(int dev)
	{
		if(online[dev-1] > 0)
			return true;
		else
			return false;
	}

	public boolean is_online()
	{
		for(int i=1;i<=parent.stations;i++)
		{
			if(online[i-1] > 0)
				return true;
		}
		return false;
	}

	private int swap(int x)
	{
		return ((x >> 8)&255)|((x & 255) << 8);
	}

	private int decodeinterval(int i)
	{
		if((i & 128)!=0)
			return (i & 127)*60;
		else
			return i;
	}

	public void openConnection()
		throws Exception
	{
		if(!mb.isOpen())
			mb.openConnection();

		poll=true;
	}

	public void closeConnection()
		throws Exception
	{
		poll=false;

		if(mb.isOpen())
			mb.closeConnection();
	}

	public void reconfigure(SerialParameters params)
		throws Exception
	{
		closeConnection();

		mb = new ModbusMaster(params, 1);
	}

	private int signed(int unsigned)
	{
		if(unsigned > 32767)
			return unsigned - 65536;
		else
			return unsigned;
	}

	private String digits(int val)
	{
		if(val < 10)
			return "0"+val;
		else
			return ""+val;
	}


	public void GetLogs(int dev,String sysname)
		throws Exception
	{

		if(logdev == 0 && is_online(dev))
		{
			logfile=new String(sysname);
			logsize=18;
			logstart=null;
			logend=null;
			logdev=dev;
		}
		else if(logdev != 0)
			throw new Exception("Already downloading logs");
	}

	public void GetLogs(int dev,String sysname,LocalDate start,LocalDate end,int log_packet_size)
		throws Exception
	{

		if(logdev == 0 && is_online(dev))
		{
			logfile=new String(sysname);
			logstart=start;
			logend=end;
			logsize=log_packet_size;
			logdev=dev;

		}

	}

	private void LogDownload(int addr)
		throws Exception
	{
			 	int day,month,year,day_no,ch,max_day,period,period_mask,max_record;
				int temp[],temp2[],chunk,chunk2,duplicates;
				Log logs[] = new Log[7*1440];
				int count=0,sleep_period;
				double measurements[];
				Date d;
				boolean clr,abort;
				LocalDate dd;

				temp = new int[2*logsize];
				temp2 = new int[5];

				measurements= new double[parent.channels+6];

				if(mb==null) return;

				mb.setDevice(addr);

				// fetch logging interval in minutes
				if(!mb.ModbusRead(40009, 1, temp))
					throw new Exception("LogDownload: Unable to read logging interval");

				sleep_period=temp[0];

				if(sleep_period < 1 || sleep_period > 48*60)
					throw new Exception("LogDownload: Invalid logging interval "+sleep_period);

				System.out.println("Sleep Period = "+sleep_period);

				if(sleep_period < 60)
				{
					max_day = 3;
					period_mask = sleep_period;
					max_record = 1440;

					duplicates=1;
				}
				else
				{
					max_day = 193;
					period_mask = sleep_period/60;

					max_record = 24 / period_mask;
					period_mask += 128;

					duplicates=1;
				}


				abort=false;

				System.out.println("Max Day    = "+max_day);
				System.out.println("Period Mask= "+period_mask);
				System.out.println("Max Record = "+max_record);
				System.out.println("Channels   = "+parent.channels);
				System.out.println("Duplicates = "+duplicates);

				if(logstart != null)
					System.out.println("Start Date = "+logstart);
				if(logend != null)
					System.out.println("End Date   = "+logend);

				Progress bar2 = new Progress(parent, 0, max_day);

				chunk2=0;

				for(day_no=0;day_no<max_day;day_no++)
				{

					if(mb==null) return;

					if(abort)
						break;

					bar2.setValue(day_no);

					if(chunk2==0)
					{
						while(!mb.ModbusRead(41001+day_no*2, logsize*2, temp))
						{
										if(0!=JOptionPane.showConfirmDialog(parent, "Download data?", "Retries exceeded .. keep trying?", JOptionPane.YES_NO_OPTION))
											{
												abort=true;
												break;
											}
						}
						chunk2=logsize;
					}

					chunk2--;

					day=(temp[(logsize-1-chunk2)*2] >> 8)&255;
					month=temp[(logsize-1-chunk2)*2] & 255;
					year=(temp[(logsize-1-chunk2)*2+1]>>8)&255;
					period=temp[(logsize-1-chunk2)*2+1] & 255;

					//if(period != 255)
						System.out.println("DAY="+day+" MONTH="+month+" YEAR="+year+" PERIOD_VAL="+period);

					try {
						dd = LocalDate.of(year+2000,month,day);
					}
					catch(DateTimeException dte)
					{
						day = 32;
						year = 100;
						month = 13;
						dd=null;
					}

					if((day < 32) && (month < 13) && (year < 99) && (year >= 14)) // && (period == period_mask))
					{

					System.out.println("LocalDate = "+dd);

					if(logstart==null|logend==null||dd==null||(logstart.isEqual(dd)||logend.isEqual(dd)||(logstart.isBefore(dd) && logend.isAfter(dd))))
					//if(0==JOptionPane.showConfirmDialog(parent, "Download data?", "Logs found for "+day+"/"+month+"/"+year+" interval "+decodeinterval(period)+" mins", JOptionPane.YES_NO_OPTION))
					{

						System.out.println("Reading day "+(day_no+1));


						while(!mb.ModbusWriteSingle(40008, day_no))
						{
										if(0!=JOptionPane.showConfirmDialog(parent, "Download data?", "Retries exceeded .. keep trying?", JOptionPane.YES_NO_OPTION))
											{
												abort=true;
												break;
											}
						}

						if(mb==null) return;

							bar = new Progress(parent, 0, max_record);

							sleep_period = decodeinterval(period);

							if(sleep_period < 60)
							{
								chunk = 16/duplicates;
								max_record = 1440;
							}
							else
							{

								max_record = 24 / (sleep_period/60);

								chunk = max_record;

								if(chunk > 10)
									chunk /= 2;
							}

							System.out.println("Chunk = "+chunk);

						 	for(int index=0;index<max_record;index+=chunk)
						 	{
									if(mb==null) return;

									// skip data beyond 1 days worth
									if(index*sleep_period >= 1440) break;

									 bar.setValue(index);

System.out.println("READ "+(parent.channels*duplicates)*chunk+" Registers");

						 			while(!mb.ModbusReadFn(4, 1002+index*(parent.channels*duplicates), (parent.channels*duplicates)*chunk, data))
									{
											if(0!=JOptionPane.showConfirmDialog(parent, "Download data?", "Retries exceeded .. keep trying?", JOptionPane.YES_NO_OPTION))
											{
												abort=true;
												break;
											}
									}


								for(int c=0;c<chunk;c++)
								{
								 for(int du=0;du<duplicates;du++)
							   	 {
								 	int isec=(index+c)*60*sleep_period+du;
									int hrs=isec/3600;
									int mins=(isec-hrs*3600)/60;
									int secs=isec-hrs*3600-mins*60;

								    clr=true;

									// only include data within correct time period of the day
									if(hrs < 24)
									{
								      for(ch=0;ch!=parent.channels;ch++)
								      {
									   if(data[ch+c*(parent.channels*duplicates)+du*parent.channels] != 65535)
									   {
										 clr=false;

											measurements[ch] = swap(data[ch+c*(parent.channels*duplicates)+du*parent.channels]);

									    }
									    else
									   	 	measurements[ch] = 0;

										if(!clr)
											System.out.println(""+hrs+":"+mins+" Channel="+ch+" Measurement="+measurements[ch]);

								 	}

								 	}

								 	if(!clr)
								 	{

										d = new Date(year+100, month-1, day, hrs, mins, secs);

										logs[count++] = new Log(d, measurements);

								 	}
								}
							   }

							} // for index

							bar.close();

						} // matched data

					} // valid date
					else
					{
						System.out.println("**SKIPPED**");
					}

				} // day number

				bar2.close();

				System.out.println("Downloaded.");

				if(count > 0)
				{
					LogChart chart;

					chart = new LogChart(parent,"Transducer Logs", logs, count);
					chart.pack();
	        		chart.setVisible(true);

				}
				else
				{
					JOptionPane.showMessageDialog(parent, "No logs found", "Download Logs", JOptionPane.INFORMATION_MESSAGE);

				}

	}

	//
	// Execute the monitor process
	//
	public void run()
	{
		int dev=1;

		if(mb.isOpen())
		{
			if(!poll)
				return;

			if(demo)
			{
				parent.callback(1,2,3,4,5,6,7,8,9,10,11,12,13,14,0);
				parent.callback2(15,16,17,18,19,20,21,22,23,24,25,26,27,28,0);
				parent.callback3(29,30,31,32,33,34,35,36,37,38,39,40,41,42,0);
				parent.set_comms_ok(true);

				for(dev=1;dev<=parent.stations;dev++)
					inc_online(dev);
			}
			else
			{

				for(dev=1;dev<=parent.stations;dev++)
				{
				mb.setDevice(dev);

				if(mb.ModbusRead(30001,21,data))
				{


					String clock = ""+digits(data[2])+":"+digits(data[1])+":"+digits(data[0])+" "+digits(data[3])+"/"+digits(data[4])+"/"+digits(data[5]);


					switch(dev)
					{
						case 1:

							parent.got_clock(clock);

							parent.callback(

										data[6+0] , //light;
										data[6+1] , //temp;
										data[6+2] , //humid;
										data[6+3] , //Voc;
										data[6+4] , //Nox;
										data[6+5] , //O2;
										data[6+6] , //NDIR;
										data[6+7] , //S302;
										data[6+8] , //S402;
										data[6+9] , //S502;
										data[6+10] , //S602;
										data[6+11] , //S702;
										data[6+12] , //S802;
										data[6+13] ,  //Voc2;
										data[6+14] // heater

									);
							break;

						case 2:

							parent.got_clock2(clock);

							parent.callback2(

										data[6+0] , //light;
										data[6+1] , //temp;
										data[6+2] , //humid;
										data[6+3] , //Voc;
										data[6+4] , //Nox;
										data[6+5] , //O2;
										data[6+6] , //NDIR;
										data[6+7] , //S302;
										data[6+8] , //S402;
										data[6+9] , //S502;
										data[6+10] , //S602;
										data[6+11] , //S702;
										data[6+12] , //S802;
										data[6+13] ,  //Voc2;
										data[6+14] // heater

									);
							break;

						case 3:

							parent.got_clock3(clock);

							parent.callback3(

										data[6+0] , //light;
										data[6+1] , //temp;
										data[6+2] , //humid;
										data[6+3] , //Voc;
										data[6+4] , //Nox;
										data[6+5] , //O2;
										data[6+6] , //NDIR;
										data[6+7] , //S302;
										data[6+8] , //S402;
										data[6+9] , //S502;
										data[6+10] , //S602;
										data[6+11] , //S702;
										data[6+12] , //S802;
										data[6+13] ,  //Voc2;
										data[6+14] // heater

									);
							break;

					}


					inc_online(dev);

					if(sleep_update)
					{
						System.out.println("Setting sleep period to "+sleep_period);

						if(mb.ModbusWriteSingle(40009, sleep_period))
						{
							System.out.println("Sleep period has been set");

							if(dev==parent.stations)
								sleep_update=false;
						}
					}

					if(erase_logs_update)
					{
						System.out.println("Erasing logs");

						if(mb.ModbusWriteSingle(40010, 5150))
						{
							if(dev==parent.stations)
								erase_logs_update=false;
						}
					}

					if(sync_clock[dev-1])
					{
						System.out.println("Sync clock");

						Date d = new Date(System.currentTimeMillis());
						mb.ModbusWriteSingle(40001, d.getSeconds());
						mb.ModbusWriteSingle(40002, d.getMinutes());
						mb.ModbusWriteSingle(40003, d.getHours());
						mb.ModbusWriteSingle(40004, d.getDate());
						mb.ModbusWriteSingle(40005, d.getMonth()+1);
						mb.ModbusWriteSingle(40006, d.getYear()+1900);
						mb.ModbusWriteSingle(40007, 1);
						sync_clock[dev-1]=false;

					}

					if(servo_update[dev-1])
					{
						System.out.println("Servo update "+servo_state[dev-1]);

						if(mb.ModbusWriteSingle(40011, servo_state[dev-1]))
							servo_update[dev-1]=false;
					}

					if(heater_update[dev-1])
					{
						System.out.println("Sensor power update "+heater_state[dev-1]);

						if(mb.ModbusWriteSingle(40013,heater_state[dev-1]))
							heater_update[dev-1]=false;
					}

					if(heater_trigger)
					{

						System.out.println("Heater Trigger");

						if(mb.ModbusWriteSingle(40012, 1))
						{
							if(dev==parent.stations)
								heater_trigger=false;
						}
					}

					parent.set_comms_ok(true);

					if(logdev != 0)
					{
					poll=false;

					try
					{
						LogDownload(dev);
					}
					catch(Exception exe)
					{
						System.out.println("EXE: "+exe.getMessage());
					}
					poll=true;


					logdev=0;
					}
				}
				else
				{
					dec_online(dev);
					parent.set_comms_ok(false);
				}
			  }
			}


		}
	}

	public void servo(int dev, int state)
	{
		servo_state[dev-1]=state;
		servo_update[dev-1]=true;
	}

	public int get_servo(int dev)
	{
		return servo_state[dev-1];
	}

	public void heater(int dev, int state)
	{
		if(heater_state[dev-1] != state)
		{
			heater_state[dev-1]=state;
			heater_update[dev-1]=true;
		}
	}

	public int get_heater(int dev)
	{
		return heater_state[dev-1];
	}

	/**
	** Cleanup the monitor process
	*/
	public void cleanup()
	{

		mb.closeConnection();

		if(logdev != 0)
		{
			logdev=0;

			bar.close();
		}
	}

	public void sync_clock(int dev)
	{
			sync_clock[dev-1]=true;

	}

	public void sleep_period_update(int period)
	{
		sleep_period = period;
		sleep_update = true;


	}

	public void erase_logs()
	{
		erase_logs_update = true;
	}

	public void heat()
	{
		heater_trigger = true;
	}

}
