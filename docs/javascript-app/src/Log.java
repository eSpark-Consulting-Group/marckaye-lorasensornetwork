
/*

Generic Logged Data Object

*/

import java.util.Date;
import java.util.Calendar;
import java.util.GregorianCalendar;

	class Log extends Object {

		private double measurement[];
		private Date timestamp;

		// return the current timezone offset in milliseconds from GMT
		public static long getZoneOffset()
		{
			Calendar c = new GregorianCalendar();

			return 	c.get(Calendar.ZONE_OFFSET) + c.get(Calendar.DST_OFFSET);
		}

		// convert the log to a string
		public String toString()
		{
			int i;
			String s=""+getChannelTimestamp()+":";


			for(i=0;i<getChannelCount();i++)
			{
				if(i > 0)
					s = s + ",";

				s = s + getChannelName(i) + "="  + getChannelValue(i) + " "+getChannelUnit(i);
			}


			return s;
		}

		public static final String measurement_names[] =
		{
			"light",
			"temp",
			"humid",
			"Voc",
			"Nox",
			"O2",
			"NDIR",
			"S302",
			"S402",
			"S502",
			"S602",
			"S702",
			"S802",
			"Voc2"
		};

		public static int getUnitCount()
		{
			return 1;
		}

		public static String getChannelUnit(int ch)
		{
			return "Raw";

		}

		public static int getChannelUnitIndex(int ch)
		{
			return 0;

		}

		public static String getChannelName(int ch)
		{
			if(ch < measurement_names.length)
				return measurement_names[ch];
			else
				return "-";
		}

		public static int getChannelCount()
		{
			return 14;
		}

		public double getChannelValue(int chan)
		{
			if(chan < getChannelCount())
				return measurement[chan];
			else
				return 0;
		}

		public Date getChannelTimestamp()
		{
			return timestamp;
		}

		public void setChannelTimestamp(Date _timestamp)
		{
			timestamp=_timestamp;
		}

		// create new log entry
		Log(java.util.Date _timestamp, double measurements[])
		{
			setChannelTimestamp(_timestamp);

			measurement = new double[getChannelCount()];

			for(int i=0;i!=getChannelCount();i++)
				measurement[i]=measurements[i];

		}

	}
