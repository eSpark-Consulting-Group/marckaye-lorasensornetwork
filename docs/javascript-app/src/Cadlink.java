
/** @(#) Cadlink.java	1.14 27/10/23 Cadlink

Title:							Cadlink
Author:							Lindsay Meek lindsay.meek@ieee.org
Description:					Windows front-end interface for Cadlink LoRa sensor interface
Development Tools:				Sun JDK, Sun JavaCOMM API, RXTX, JAMOD

*/

import java.awt.*;
import java.awt.event.*;
import java.util.Hashtable;
import java.io.*;
import javax.swing.*;
import javax.swing.text.*;
import javax.swing.event.*;
import javax.swing.undo.*;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.FileNotFoundException;
import java.util.Properties;
import java.util.Timer;
import javax.swing.plaf.metal.MetalBorders;
import java.util.Date;
import java.util.TimerTask;
import java.util.Timer;

import java.net.URL;
import java.net.HttpURLConnection;
import java.net.MalformedURLException;
import java.net.ProtocolException;
import java.time.LocalDateTime;

public class Cadlink extends JFrame  {

    private final String newline = "\n";
	JLabel statusLabel;
	private String comport;
	private boolean started;
	private AboutAction aboutAction;
	private ConnectAction connectAction;
	private DisconnectAction disconnectAction;
	private Cadlink self;
	private JMenu configMenu,fileMenu;
	private JMenuBar mb;
	private Properties props;
	private	JRadioButtonMenuItem mcomi[],mspeedi[];
	private SerialParameters params;
	private Timer tmr;
	private final String on_off[] = { "OFF", "ON" };
	private Monitor mon;
	private JButton comms_light,power_light;
	private boolean scan,timer_not_started;
	private ImageIcon light_green,light_red,logo,light_on,light_off,light_yellow,light_blue,light_white;
	private ExitAction exitAction;
	private Font titleFont,titleFont2,buttonFont,buttonFont2,buttonFont3;
	private JPanel statusPane;
	private JLabel rtc,rtc2,rtc3;
	private JLabel light;
	private JLabel temp;
	private JLabel humid;
	private JLabel Voc;
	private JLabel Nox;
	private JLabel O2;
	private JLabel NDIR;
	private JLabel S302;
	private JLabel S402;
	private JLabel S502;
	private JLabel S602;
	private JLabel S702;
	private JLabel S802;
	private JLabel VocB;
	private JLabel light2;
	private JLabel temp2;
	private JLabel humid2;
	private JLabel Voc2;
	private JLabel Nox2;
	private JLabel O22;
	private JLabel NDIR2;
	private JLabel S3022;
	private JLabel S4022;
	private JLabel S5022;
	private JLabel S6022;
	private JLabel S7022;
	private JLabel S8022;
	private JLabel VocB2;
	private JLabel light3;
	private JLabel temp3;
	private JLabel humid3;
	private JLabel Voc3;
	private JLabel Nox3;
	private JLabel O23;
	private JLabel NDIR3;
	private JLabel S3023;
	private JLabel S4023;
	private JLabel S5023;
	private JLabel S6023;
	private JLabel S7023;
	private JLabel S8023;
	private JLabel VocB3;
	private JLabel GPS_LAT,GPS_LON;
	private JLabel power1,power2,power3;
	private double LAT,LON,FirstPhase,SecondPhase,ThirdPhase;

	public final int stations=3;
	public final int channels=14;
	private TrendChart trend,trend2,trend3;
	private int poll;
	private SendSensors gsc;

	private boolean fumigation;
	private int fumigation_timer,light_timer;
	private JLabel fum_status;
	private Timer tmr2;
	private PollThread thr;

	private final int poll_times[] = { 1, 2, 5, 10, 30, 60 };

	private void announce(String unit)
	{

		try {

		String currentTime = ""+java.time.LocalDateTime.now();

		URL url = new URL("http://cms9.southeastasia.cloudapp.azure.com/IOTApi/api/DeviceInfo/UpdateFumigationStart/"+unit+"/"+currentTime);

System.out.println("OPEN "+url);

    	HttpURLConnection httpCon = (HttpURLConnection) url.openConnection();

System.out.println("SEND");

    	httpCon.setDoOutput(true);
    	httpCon.setRequestMethod("PUT");
    	OutputStreamWriter out = new OutputStreamWriter(httpCon.getOutputStream());
    	out.write("");

System.out.println("CLOSE");

    	out.close();
    	httpCon.getInputStream();
		} catch (MalformedURLException e) {
    		e.printStackTrace();
		} catch (ProtocolException e) {
		    e.printStackTrace();
		} catch (IOException e) {
		    e.printStackTrace();
		}

	}

	private void setLAT(double _LAT)
	{
		LAT=_LAT;

		GPS_LAT.setText("<HTML><CENTER>LAT: "+LAT+"</CENTER></HTML>");

		gsc.LAT = LAT;


	}

	private void setLON(double _LON)
	{
		LON=_LON;

		GPS_LON.setText("<HTML><CENTER>LON: "+LON+"</CENTER></HTML>");

		gsc.LON = LON;
	}


   /**
     * Constructs a <code>Cadlink</code> object
     * connected to the nominated com port
     *
     * @param   port		The name of the com port
     *
     */
    public Cadlink(String port) {

        //some initial setup
        super("LoRa Sensor Board");

		int i,j,k;

		comport=port;

		started=false;

		self=this;

		params = new SerialParameters();

		scan=false;

		poll=60;

		LAT = -31.9535132;
		LON = 115.86140000;

		FirstPhase=2;
		SecondPhase=4;
		ThirdPhase=12;

		fumigation=false;
		fumigation_timer=0;

		gsc = new SendSensors();

		setIconImage(new ImageIcon("icon.gif").getImage());

		setBackground(Color.white);

		titleFont = new Font("Arial", Font.PLAIN, 14);
		titleFont2 = new Font("Arial", Font.BOLD, 14);
		buttonFont = new Font("Arial", Font.BOLD, 14);
		buttonFont2 = new Font("Courier", Font.BOLD, 14);
		buttonFont3 = new Font("Arial", Font.BOLD, 18);
		Font dialFont = new Font("Courier", Font.BOLD, 40);

		light_yellow = new ImageIcon("light_yellow.gif");
		light_white = new ImageIcon("light_white.gif");
		light_blue = new ImageIcon("light_blue.gif");
		light_on = new ImageIcon("light_on.gif");
		light_off = new ImageIcon("light_off.gif");
		light_red = new ImageIcon("red.gif");
		light_green = new ImageIcon("green.gif");

		comms_light = new JButton("COMMUNICATION",light_red);
		comms_light.setVerticalTextPosition(SwingConstants.TOP);
		comms_light.setHorizontalTextPosition(SwingConstants.CENTER);
		comms_light.setFont(buttonFont);
		comms_light.setContentAreaFilled(false);
		comms_light.setBorderPainted(false);
		comms_light.setRolloverEnabled(true);
		comms_light.setFocusPainted(false);


		GPS_LAT = new JLabel("", SwingConstants.CENTER);
		GPS_LON = new JLabel("", SwingConstants.CENTER);
		GPS_LAT.setFont(buttonFont3);
		GPS_LON.setFont(buttonFont3);
		setLAT(LAT);
		setLON(LON);

        //Set up the menu bar.

		fileMenu = createFileMenu();

        JMenu aboutMenu = new JMenu("About");
        aboutAction = new AboutAction();
        aboutMenu.add(aboutAction);
        mb = new JMenuBar();
		mb.add(fileMenu);

		mb.add(aboutMenu);
        setJMenuBar(mb);

		pack();

	 	tmr = new Timer();

		params.setPortName(comport);

		mon = new Monitor(this, params);

		self.setIconImage(new ImageIcon("icon.gif").getImage());
		self.setBackground(Color.white);

		//Create the title area
		JPanel titlePane = new JPanel(new GridLayout(1,1));
		titlePane.setBackground(Color.white);

		JLabel t1Label = new JLabel("<HTML><CENTER>LORA SENSOR BOARD</CENTER></HTML>", SwingConstants.CENTER);
		t1Label.setFont(dialFont);
		titlePane.add(t1Label);

		statusPane = new JPanel(new GridLayout(15, 4));
		statusPane.setBackground(Color.white);

		rtc = new JLabel("<HTML><CENTER>00:00:00 00/00/00</CENTER></HTML>", SwingConstants.CENTER);
		rtc.setFont(new Font("Arial", Font.BOLD, 8));

		rtc2 = new JLabel("<HTML><CENTER>00:00:00 00/00/00</CENTER></HTML>", SwingConstants.CENTER);
		rtc2.setFont(new Font("Arial", Font.BOLD, 8));

		rtc3 = new JLabel("<HTML><CENTER>00:00:00 00/00/00</CENTER></HTML>", SwingConstants.CENTER);
		rtc3.setFont(new Font("Arial", Font.BOLD, 8));

		light = new JLabel("-", SwingConstants.CENTER);
		temp = new JLabel("-", SwingConstants.CENTER);
		humid = new JLabel("-", SwingConstants.CENTER);
		Voc = new JLabel("-", SwingConstants.CENTER);
		Nox = new JLabel("-", SwingConstants.CENTER);
		O2 = new JLabel("-", SwingConstants.CENTER);
		NDIR = new JLabel("-", SwingConstants.CENTER);
		S302 = new JLabel("-", SwingConstants.CENTER);
		S402 = new JLabel("-", SwingConstants.CENTER);
		S502 = new JLabel("-", SwingConstants.CENTER);
		S602 = new JLabel("-", SwingConstants.CENTER);
		S702 = new JLabel("-", SwingConstants.CENTER);
		S802 = new JLabel("-", SwingConstants.CENTER);
		VocB = new JLabel("-", SwingConstants.CENTER);

		rtc.setFont(buttonFont3);
		rtc2.setFont(buttonFont3);
		rtc3.setFont(buttonFont3);
		light.setFont(buttonFont3);
		temp.setFont(buttonFont3);
		humid.setFont(buttonFont3);
		Voc.setFont(buttonFont3);
		Nox.setFont(buttonFont3);
		O2.setFont(buttonFont3);
		NDIR.setFont(buttonFont3);
		S302.setFont(buttonFont3);
		S402.setFont(buttonFont3);
		S502.setFont(buttonFont3);
		S602.setFont(buttonFont3);
		S702.setFont(buttonFont3);
		S802.setFont(buttonFont3);
		VocB.setFont(buttonFont3);

		light2 = new JLabel("-", SwingConstants.CENTER);
		temp2 = new JLabel("-", SwingConstants.CENTER);
		humid2 = new JLabel("-", SwingConstants.CENTER);
		Voc2 = new JLabel("-", SwingConstants.CENTER);
		Nox2 = new JLabel("-", SwingConstants.CENTER);
		O22 = new JLabel("-", SwingConstants.CENTER);
		NDIR2 = new JLabel("-", SwingConstants.CENTER);
		S3022 = new JLabel("-", SwingConstants.CENTER);
		S4022 = new JLabel("-", SwingConstants.CENTER);
		S5022 = new JLabel("-", SwingConstants.CENTER);
		S6022 = new JLabel("-", SwingConstants.CENTER);
		S7022 = new JLabel("-", SwingConstants.CENTER);
		S8022 = new JLabel("-", SwingConstants.CENTER);
		VocB2 = new JLabel("-", SwingConstants.CENTER);

		light2.setFont(buttonFont3);
		temp2.setFont(buttonFont3);
		humid2.setFont(buttonFont3);
		Voc2.setFont(buttonFont3);
		Nox2.setFont(buttonFont3);
		O22.setFont(buttonFont3);
		NDIR2.setFont(buttonFont3);
		S3022.setFont(buttonFont3);
		S4022.setFont(buttonFont3);
		S5022.setFont(buttonFont3);
		S6022.setFont(buttonFont3);
		S7022.setFont(buttonFont3);
		S8022.setFont(buttonFont3);
		VocB2.setFont(buttonFont3);

		light3 = new JLabel("-", SwingConstants.CENTER);
		temp3 = new JLabel("-", SwingConstants.CENTER);
		humid3 = new JLabel("-", SwingConstants.CENTER);
		Voc3 = new JLabel("-", SwingConstants.CENTER);
		Nox3 = new JLabel("-", SwingConstants.CENTER);
		O23 = new JLabel("-", SwingConstants.CENTER);
		NDIR3 = new JLabel("-", SwingConstants.CENTER);
		S3023 = new JLabel("-", SwingConstants.CENTER);
		S4023 = new JLabel("-", SwingConstants.CENTER);
		S5023 = new JLabel("-", SwingConstants.CENTER);
		S6023 = new JLabel("-", SwingConstants.CENTER);
		S7023 = new JLabel("-", SwingConstants.CENTER);
		S8023 = new JLabel("-", SwingConstants.CENTER);
		VocB3 = new JLabel("-", SwingConstants.CENTER);

		light3.setFont(buttonFont3);
		temp3.setFont(buttonFont3);
		humid3.setFont(buttonFont3);
		Voc3.setFont(buttonFont3);
		Nox3.setFont(buttonFont3);
		O23.setFont(buttonFont3);
		NDIR3.setFont(buttonFont3);
		S3023.setFont(buttonFont3);
		S4023.setFont(buttonFont3);
		S5023.setFont(buttonFont3);
		S6023.setFont(buttonFont3);
		S7023.setFont(buttonFont3);
		S8023.setFont(buttonFont3);
		VocB3.setFont(buttonFont3);

		power1 = new JLabel("-", SwingConstants.CENTER);
		power1.setFont(buttonFont3);
		power2 = new JLabel("-", SwingConstants.CENTER);
		power2.setFont(buttonFont3);
		power3 = new JLabel("-", SwingConstants.CENTER);
		power3.setFont(buttonFont3);

		statusPane.add(new JLabel("LIGHT", SwingConstants.CENTER));
		statusPane.add(light);
		statusPane.add(light2);
		statusPane.add(light3);
		statusPane.add(new JLabel("TEMP", SwingConstants.CENTER));
		statusPane.add(temp);
		statusPane.add(temp2);
		statusPane.add(temp3);
		statusPane.add(new JLabel("HUMIDITY", SwingConstants.CENTER));
		statusPane.add(humid);
		statusPane.add(humid2);
		statusPane.add(humid3);
		statusPane.add(new JLabel("VOC-SGP41", SwingConstants.CENTER));
		statusPane.add(Voc);
		statusPane.add(Voc2);
		statusPane.add(Voc3);
		statusPane.add(new JLabel("NOX-SGP41", SwingConstants.CENTER));
		statusPane.add(Nox);
		statusPane.add(Nox2);
		statusPane.add(Nox3);
		statusPane.add(new JLabel("O2", SwingConstants.CENTER));
		statusPane.add(O2);
		statusPane.add(O22);
		statusPane.add(O23);
		statusPane.add(new JLabel("NDIR", SwingConstants.CENTER));
		statusPane.add(NDIR);
		statusPane.add(NDIR2);
		statusPane.add(NDIR3);
		statusPane.add(new JLabel("S302", SwingConstants.CENTER));
		statusPane.add(S302);
		statusPane.add(S3022);
		statusPane.add(S3023);
		statusPane.add(new JLabel("S402", SwingConstants.CENTER));
		statusPane.add(S402);
		statusPane.add(S4022);
		statusPane.add(S4023);
		statusPane.add(new JLabel("S502", SwingConstants.CENTER));
		statusPane.add(S502);
		statusPane.add(S5022);
		statusPane.add(S5023);
		statusPane.add(new JLabel("S602", SwingConstants.CENTER));
		statusPane.add(S602);
		statusPane.add(S6022);
		statusPane.add(S6023);
		statusPane.add(new JLabel("S702", SwingConstants.CENTER));
		statusPane.add(S702);
		statusPane.add(S7022);
		statusPane.add(S7023);
		statusPane.add(new JLabel("S802", SwingConstants.CENTER));
		statusPane.add(S802);
		statusPane.add(S8022);
		statusPane.add(S8023);
		statusPane.add(new JLabel("VOC-SGP40", SwingConstants.CENTER));
		statusPane.add(VocB);
		statusPane.add(VocB2);
		statusPane.add(VocB3);
		statusPane.add(new JLabel("POWER", SwingConstants.CENTER));
		statusPane.add(power1);
		statusPane.add(power2);
		statusPane.add(power3);

		//Create the clock panel
		JPanel clockPane = new JPanel(new GridLayout(2, 10));

		clockPane.setBackground(Color.white);

		clockPane.add(rtc);
		clockPane.add(rtc2);
		clockPane.add(rtc3);
		clockPane.add(comms_light);

		JButton sync = new JButton("Sync Clocks");
		sync.addActionListener(new ButtonAction(0,0,"sync"));
		sync.setFont(buttonFont);
		clockPane.add(sync);
		JButton launch = new JButton("Start Onboard Logging");
		launch.addActionListener(new ButtonAction(0,0,"launch"));
		launch.setFont(buttonFont);
		clockPane.add(launch);

		JButton startf = new JButton("Start Fumigation");
		startf.addActionListener(new ButtonAction(0,0,"start"));
		startf.setFont(buttonFont);
		clockPane.add(startf);

		clockPane.add(GPS_LON);

		fum_status = new JLabel("0:0:0", SwingConstants.CENTER);
		fum_status.setFont(buttonFont3);
		clockPane.add(fum_status);

		JButton download = new JButton("Download Logs");
		download.addActionListener(new ButtonAction(0,0,"download"));
		download.setFont(buttonFont);
		clockPane.add(download);

		JButton erase = new JButton("Erase Logs");
		erase.addActionListener(new ButtonAction(0,0,"erase"));
		erase.setFont(buttonFont);
		clockPane.add(erase);

		JButton servo_off = new JButton("Open Covers");
		servo_off.addActionListener(new ButtonAction(0,0,"servo_off"));
		servo_off.setFont(buttonFont);
		clockPane.add(servo_off);


		JButton servo_on = new JButton("Close Covers");
		servo_on.addActionListener(new ButtonAction(0,0,"servo_on"));
		servo_on.setFont(buttonFont);
		clockPane.add(servo_on);

		JButton save = new JButton("Save Trends");
		save.addActionListener(new ButtonAction(0,0,"save"));
		save.setFont(buttonFont);
		clockPane.add(save);

		JButton heat = new JButton("Heat SHT45s");
		heat.addActionListener(new ButtonAction(0,0,"heat"));
		heat.setFont(buttonFont);
		clockPane.add(heat);

		JButton stopf = new JButton("Stop Fumigation");
		stopf.addActionListener(new ButtonAction(0,0,"stop"));
		stopf.setFont(buttonFont);
		clockPane.add(stopf);

		clockPane.add(GPS_LAT);

		JButton skipf = new JButton("Start Sleep");
		skipf.addActionListener(new ButtonAction(0,0,"sleep"));
		skipf.setFont(buttonFont);
		clockPane.add(skipf);

		JPanel trendPane = new JPanel(new GridLayout(1,1));
		trendPane.setBackground(Color.white);

		trend = new TrendChart(400,200,"Time Series1", trendPane);
		trend2 = new TrendChart(400,200,"Time Series2", trendPane);
		trend3 = new TrendChart(400,200,"Time Series3", trendPane);


       //Add the components to the frame.
        JPanel contentPane = new JPanel(new BorderLayout());
		contentPane.setBackground(Color.white);
		contentPane.add(titlePane, BorderLayout.NORTH);
		contentPane.add(statusPane, BorderLayout.CENTER);
		contentPane.add(clockPane, BorderLayout.SOUTH);
		contentPane.add(trendPane, BorderLayout.EAST);

        setContentPane(contentPane);

		thr = new PollThread();
		tmr2 = new Timer("Fumigation_Poll");
		tmr2.scheduleAtFixedRate(thr,0,1000);

		pack();

		setVisible(true);

    }

	void set_comms_ok(boolean ok)
	{
			if(!ok)
					comms_light.setIcon(light_red);
				else
					comms_light.setIcon(light_green);
	}


    /**
	 * 	The standard main method.
	 * @param args Program arguments
	**/
    public static void main(String[] args) {

		// enable anti-aliased text:
  		System.setProperty("awt.useSystemAAFontSettings","on");
  		System.setProperty("swing.aatext", "true");

        final Cadlink frame = new Cadlink("COM1");
        frame.addWindowListener(new WindowAdapter() {
            public void windowClosing(WindowEvent e) {
                System.exit(0);
            }

        });

        frame.pack();
        frame.setVisible(true);
    }


    // Create the file menu.
    private JMenu createFileMenu() {
		int i;

        JMenu menu = new JMenu("File");

		JMenu mcom = new JMenu("COM Port");

		ButtonGroup group1 = new ButtonGroup();

		mcomi = new JRadioButtonMenuItem[32];

		for(i=0;i<32;i++)
		{
			mcomi[i] = new JRadioButtonMenuItem("COM"+(i+1));

			mcomi[i].addActionListener(new ButtonAction(0,0,"COM"+(i+1)));

			if(comport.equals("COM"+(i+1)))
				mcomi[i].setSelected(true);
			else
				mcomi[i].setSelected(false);

			group1.add(mcomi[i]);

			mcom.add(mcomi[i]);

		}

		ButtonGroup group2 = new ButtonGroup();

		JMenu mspeed = new JMenu("Poll Rate");

		mspeedi = new JRadioButtonMenuItem[6];

		for(i=0;i<6;i++)
		{
			mspeedi[i] = new JRadioButtonMenuItem(""+poll_times[i]+"s");

			mspeedi[i].addActionListener(new ButtonAction(0,0,"POLL"+(i+1)));

			if(poll == poll_times[i])
				mspeedi[i].setSelected(true);
			else
				mspeedi[i].setSelected(false);

			group2.add(mspeedi[i]);

			mspeed.add(mspeedi[i]);

		}

		JMenu gps = new JMenu("GPS");

		JMenuItem gps_lat = new JMenuItem("Set Default Latitude");
		gps_lat.addActionListener(new ButtonAction(0,0,"lat"));
		gps.add(gps_lat);
		JMenuItem gps_lon = new JMenuItem("Set Default Longitude");
		gps_lon.addActionListener(new ButtonAction(0,0,"lon"));
		gps.add(gps_lon);

        connectAction = new ConnectAction();
        menu.add(connectAction);
        disconnectAction = new DisconnectAction();
        menu.add(disconnectAction);

		menu.add(mcom);

		menu.add(mspeed);

		menu.add(gps);

		JMenu timing = new JMenu("Timing");
		JMenuItem set_first = new JMenuItem("Set First Phase Duration");
		set_first.addActionListener(new ButtonAction(0,0,"first"));
		timing.add(set_first);
		JMenuItem set_second = new JMenuItem("Set Second Phase Duration");
		set_second.addActionListener(new ButtonAction(0,0,"second"));
		timing.add(set_second);
		JMenuItem set_third = new JMenuItem("Set Third Phase Duration");
		set_third.addActionListener(new ButtonAction(0,0,"third"));
		timing.add(set_third);

		menu.add(timing);

        exitAction = new ExitAction();
        menu.add(exitAction);

		LoadConfig();

		timer_not_started=true;

        return menu;
    }



	//
	// Handler for connect
	//
	class ConnectAction extends AbstractAction {

		public ConnectAction() {
            super("Connect");
            setEnabled(true);
        	}

        public void actionPerformed(ActionEvent e) {

		try
		{
				if(timer_not_started)
				{
					tmr.scheduleAtFixedRate(mon, 0, poll*1000);
					timer_not_started=false;
				}

				mon.openConnection();

				gsc.LAT = LAT;
				gsc.LON = LON;

				gsc.connect();

				set_comms_ok(true);


		} catch(Exception ex)
		{
			JOptionPane.showMessageDialog(self, ex.getMessage(), "Unable to Connect", JOptionPane.ERROR_MESSAGE);

			try { mon.closeConnection(); } catch(Exception ex2) {  }

			set_comms_ok(false);

		}

		}
    }

	//
	// Handler for disconnect
	//
	class DisconnectAction extends AbstractAction {

		public DisconnectAction() {
            super("Disconnect");
            setEnabled(true);
        	}

        public void actionPerformed(ActionEvent e) {

		try
		{

		mon.closeConnection();
		set_comms_ok(false);

		} catch(Exception ex)
		{
			JOptionPane.showMessageDialog(self, ex.getMessage(), "Unable to Disconnect", JOptionPane.ERROR_MESSAGE);

		}

		}
    }

	//
	// Handler for about
	//
	class AboutAction extends AbstractAction {

		public AboutAction() {
            super("About");
            setEnabled(true);
        	}

        public void actionPerformed(ActionEvent e) {

			JOptionPane.showMessageDialog(self, "Version 1.23", "LoRa Sensor Board Control Panel", JOptionPane.INFORMATION_MESSAGE);


		}
    }

	private final String configFileName = "c:\\program files\\LoRaSensorBoard\\config.txt";

	//private final String configFileName = "config.txt";

	// Load application configuration
	private void LoadConfig()
	{
		String s;
        File f = new File(configFileName);
		int i,j;

        try {

		System.out.println("Loading config from "+configFileName);

       	FileInputStream fis = new FileInputStream(f);
        Properties p = new Properties();
        p.load(fis);
        fis.close();

		comport="COM1";
		s=p.getProperty("Port");
		if(s!=null)
		{
			i=Integer.parseInt(s);
			if(i >= 1 && i <= 32)
			{
				for(j=0;j<32;j++)
					mcomi[j].setSelected((j+1)==i);

				comport="COM"+i;
			}

		}

		s=p.getProperty("Poll");
		if(s!=null)
		{
			i=Integer.parseInt(s);

			for(j=0;j<6;j++)
				mspeedi[j].setSelected(poll_times[j]==i);

			poll=i;

		}

		 s=p.getProperty("LAT");
		 if(s!=null)
		 {
			LAT = Double.parseDouble(s);

			setLAT(LAT);
		 }

		 s=p.getProperty("LON");
		 if(s!=null)
		 {
			LON = Double.parseDouble(s);

			setLON(LON);
		 }


		 s=p.getProperty("FirstPhase");
		 if(s!=null)
		 {
			FirstPhase = Double.parseDouble(s);
		 }

		 s=p.getProperty("SecondPhase");
		 if(s!=null)
		 {
			SecondPhase = Double.parseDouble(s);
		 }

		 s=p.getProperty("ThirdPhase");
		 if(s!=null)
		 {
			ThirdPhase = Double.parseDouble(s);
		 }

		}
		catch(IOException e)
		{
			System.out.println("Unable to load config - "+e.getMessage());
		}

	}

		// convert double to string with a fixed # of decimal places
		private String toStringDP(double v,int dp)
		{
			String t;

			if(dp==0)
			{
				t=Double.toString(Math.rint(v));

				return t.substring(0,t.indexOf('.'));

			}
			else
				return Double.toString(Math.rint(Math.pow(10.0,dp)*v)/Math.pow(10.0,dp));

		}



	public void got_clock(String clk)
	{
		rtc.setText("<HTML><CENTER>"+clk+"</CENTER></HTML>");


	}


	public void got_clock2(String clk)
	{
		rtc2.setText("<HTML><CENTER>"+clk+"</CENTER></HTML>");


	}


	public void got_clock3(String clk)
	{
		rtc3.setText("<HTML><CENTER>"+clk+"</CENTER></HTML>");


	}

	public void callback(						int _light,
												int _temp,
												int _humid,
												int _Voc,
												int _Nox,
												int _O2,
												int _NDIR,
												int _S302,
												int _S402,
												int _S502,
												int _S602,
												int _S702,
												int _S802,
											  	int _Voc2,
											  	int _heater)
	{

												light.setText(""+_light);
												temp.setText(""+_temp);
												humid.setText(""+_humid);
												Voc.setText(""+_Voc);
												Nox.setText(""+_Nox);
												O2.setText(""+_O2);
												NDIR.setText(""+_NDIR);
												S302.setText(""+_S302);
												S402.setText(""+_S402);
												S502.setText(""+_S502);
												S602.setText(""+_S602);
												S702.setText(""+_S702);
												S802.setText(""+_S802);
											  	VocB.setText(""+_Voc2);
												power1.setText(""+_heater);

												gsc.light1=_light;
												gsc.temp1=_temp;
												gsc.humid1=_humid;
												gsc.Voc1=_Voc;
												gsc.Nox1=_Nox;
												gsc.O21=_O2;
												gsc.NDIR1=_NDIR;
												gsc.S3021=_S302;
												gsc.S4021=_S402;
												gsc.S5021=_S502;
												gsc.S6021=_S602;
												gsc.S7021=_S702;
												gsc.S8021=_S802;
											  	gsc.VocB1=_Voc2;
												gsc.servo1=mon.get_servo(1);

		if(gsc == null || gsc.active)
		{
		trend.add_data(1,_light);
		trend.add_data(2,_temp);
		trend.add_data(3,_humid);
		trend.add_data(4,_Voc);
		trend.add_data(5,_Nox);
		trend.add_data(6,_O2);
		trend.add_data(7,_NDIR);
		trend.add_data(8,_S302);
		trend.add_data(9,_S402);
		trend.add_data(10,_S502);
		trend.add_data(11,_S602);
		trend.add_data(12,_S702);
		trend.add_data(13,_S802);
		trend.add_data(14,_Voc2);
		}

	}

	public void callback2(						int _light,
												int _temp,
												int _humid,
												int _Voc,
												int _Nox,
												int _O2,
												int _NDIR,
												int _S302,
												int _S402,
												int _S502,
												int _S602,
												int _S702,
												int _S802,
											  	int _Voc2,
											  	int _heater)
	{

												light2.setText(""+_light);
												temp2.setText(""+_temp);
												humid2.setText(""+_humid);
												Voc2.setText(""+_Voc);
												Nox2.setText(""+_Nox);
												O22.setText(""+_O2);
												NDIR2.setText(""+_NDIR);
												S3022.setText(""+_S302);
												S4022.setText(""+_S402);
												S5022.setText(""+_S502);
												S6022.setText(""+_S602);
												S7022.setText(""+_S702);
												S8022.setText(""+_S802);
											  	VocB2.setText(""+_Voc2);
												power2.setText(""+_heater);

												gsc.light2=_light;
												gsc.temp2=_temp;
												gsc.humid2=_humid;
												gsc.Voc2=_Voc;
												gsc.Nox2=_Nox;
												gsc.O22=_O2;
												gsc.NDIR2=_NDIR;
												gsc.S3022=_S302;
												gsc.S4022=_S402;
												gsc.S5022=_S502;
												gsc.S6022=_S602;
												gsc.S7022=_S702;
												gsc.S8022=_S802;
											  	gsc.VocB2=_Voc2;
												gsc.servo2=mon.get_servo(2);

		if(gsc == null || gsc.active)
		{
		trend2.add_data(1,_light);
		trend2.add_data(2,_temp);
		trend2.add_data(3,_humid);
		trend2.add_data(4,_Voc);
		trend2.add_data(5,_Nox);
		trend2.add_data(6,_O2);
		trend2.add_data(7,_NDIR);
		trend2.add_data(8,_S302);
		trend2.add_data(9,_S402);
		trend2.add_data(10,_S502);
		trend2.add_data(11,_S602);
		trend2.add_data(12,_S702);
		trend2.add_data(13,_S802);
		trend2.add_data(14,_Voc2);
		}

	}

	public void callback3(						int _light,
												int _temp,
												int _humid,
												int _Voc,
												int _Nox,
												int _O2,
												int _NDIR,
												int _S302,
												int _S402,
												int _S502,
												int _S602,
												int _S702,
												int _S802,
											  	int _Voc2,
											  	int _heater)
	{

												light3.setText(""+_light);
												temp3.setText(""+_temp);
												humid3.setText(""+_humid);
												Voc3.setText(""+_Voc);
												Nox3.setText(""+_Nox);
												O23.setText(""+_O2);
												NDIR3.setText(""+_NDIR);
												S3023.setText(""+_S302);
												S4023.setText(""+_S402);
												S5023.setText(""+_S502);
												S6023.setText(""+_S602);
												S7023.setText(""+_S702);
												S8023.setText(""+_S802);
											  	VocB3.setText(""+_Voc2);
												power3.setText(""+_heater);

												gsc.light3=_light;
												gsc.temp3=_temp;
												gsc.humid3=_humid;
												gsc.Voc3=_Voc;
												gsc.Nox3=_Nox;
												gsc.O23=_O2;
												gsc.NDIR3=_NDIR;
												gsc.S3023=_S302;
												gsc.S4023=_S402;
												gsc.S5023=_S502;
												gsc.S6023=_S602;
												gsc.S7023=_S702;
												gsc.S8023=_S802;
											  	gsc.VocB3=_Voc2;
												gsc.servo3=mon.get_servo(3);


		if(gsc == null || gsc.active)
		{
		trend3.add_data(1,_light);
		trend3.add_data(2,_temp);
		trend3.add_data(3,_humid);
		trend3.add_data(4,_Voc);
		trend3.add_data(5,_Nox);
		trend3.add_data(6,_O2);
		trend3.add_data(7,_NDIR);
		trend3.add_data(8,_S302);
		trend3.add_data(9,_S402);
		trend3.add_data(10,_S502);
		trend3.add_data(11,_S602);
		trend3.add_data(12,_S702);
		trend3.add_data(13,_S802);
		trend3.add_data(14,_Voc2);
		}

	}

	// Save application configuration
	private void SaveConfig()
	{
        	 Properties newProps;
        	 FileOutputStream fileOut = null;
			 int i;

        	 newProps = new Properties();

			// extract the selected com ports
			for(i=0;i<32;i++)
			{
				if(mcomi[i].isSelected())
					break;
			}
			newProps.put("Port", ""+(i+1));

			newProps.put("Poll", ""+poll);

			newProps.put("LAT", ""+LAT);

			newProps.put("LON", ""+LON);

			newProps.put("FirstPhase", ""+FirstPhase);

			newProps.put("SecondPhase", ""+SecondPhase);

			newProps.put("ThirdPhase", ""+ThirdPhase);

                try
                {
					System.out.println("Saving config to "+configFileName);
                    fileOut = new FileOutputStream(configFileName);
                    newProps.save(fileOut, "Application Configuration");
                    fileOut.close();

                }
                catch (IOException e2)
                {
					System.out.println("Unable to save config - "+e2.getMessage());
                }
	}

	//
	// Handler for exit
	//
	class ExitAction extends AbstractAction {

		public ExitAction() {
            super("Quit");
            setEnabled(true);
        	}

        public void actionPerformed(ActionEvent e) {

		try
		{
			set_comms_ok(false);

			mon.closeConnection();

		} catch(Exception ex)
		{

		}

		System.exit(0);
		}
    }

		//
		// Handler for button presses
		//
		class ButtonAction extends AbstractAction
		{
			int i,arg1,arg2;
			String name;

			public ButtonAction(int _arg1,int _arg2,String _name)
			{

				super("Button");

				arg1=_arg1; arg2=_arg2;
				name=_name;

				setEnabled(true);

			}

			public void actionPerformed(ActionEvent e)
			{

				if(arg1==0 && arg2==0)
				{


					if(name.contains("POLL"))
					{
						switch(name)
						{

							case "POLL1": poll=poll_times[0]; break;
							case "POLL2": poll=poll_times[1]; break;
							case "POLL3": poll=poll_times[2]; break;
							case "POLL4": poll=poll_times[3]; break;
							case "POLL5": poll=poll_times[4]; break;

							default:
							case "POLL6": poll=poll_times[5]; break;


						}

						SaveConfig();
					}
					else if(name.contains("COM"))
					{
						comport = name;

						SaveConfig();

						params.setPortName(comport);

						try
						{
							mon.reconfigure(params);
						}
						catch(Exception sce)
						{
							JOptionPane.showMessageDialog(self, sce.getMessage(), "Serial Exception", JOptionPane.ERROR_MESSAGE);
						}
					}
					else
					{

						switch(name)
						{
								default:
									System.out.println("Unhandled action: "+name);
									break;

								case "first":

									String firsts = JOptionPane.showInputDialog(self, "Please enter the First Phase Duration (Hrs)", ""+FirstPhase);

									if(firsts!=null)
									{
										try
										{
											double firstn = Double.parseDouble(firsts);

											if(firstn < 1 || firstn > 12)
											{
												JOptionPane.showMessageDialog(self, "Needs to be between 1 and 12 hours", "First Phase out of range", JOptionPane.ERROR_MESSAGE);
												return;
											}

											FirstPhase=firstn;
											SaveConfig();

										}
										catch(NumberFormatException nfe)
										{
											JOptionPane.showMessageDialog(self, "Invalid Number "+firsts, "First Phase out of range", JOptionPane.ERROR_MESSAGE);
											return;
										}

									}
									break;

								case "second":

									String seconds = JOptionPane.showInputDialog(self, "Please enter the Second Phase Duration (Hrs)", ""+SecondPhase);

									if(seconds!=null)
									{
										try
										{
											double secondn = Double.parseDouble(seconds);

											if(secondn < 1 || secondn > 12)
											{
												JOptionPane.showMessageDialog(self, "Needs to be between 1 and 12 hours", "Second Phase out of range", JOptionPane.ERROR_MESSAGE);
												return;
											}

											SecondPhase=secondn;
											SaveConfig();

										}
										catch(NumberFormatException nfe)
										{
											JOptionPane.showMessageDialog(self, "Invalid Number "+seconds, "Second Phase out of range", JOptionPane.ERROR_MESSAGE);
											return;
										}

									}
									break;

								case "third":

									String thirds = JOptionPane.showInputDialog(self, "Please enter the Third Phase Duration (Hrs)", ""+SecondPhase);

									if(thirds!=null)
									{
										try
										{
											double thirdn = Double.parseDouble(thirds);

											if(thirdn < 1 || thirdn > 24)
											{
												JOptionPane.showMessageDialog(self, "Needs to be between 1 and 24 hours", "Third Phase out of range", JOptionPane.ERROR_MESSAGE);
												return;
											}

											ThirdPhase=thirdn;
											SaveConfig();

										}
										catch(NumberFormatException nfe)
										{
											JOptionPane.showMessageDialog(self, "Invalid Number "+thirds, "Third Phase out of range", JOptionPane.ERROR_MESSAGE);
											return;
										}

									}
									break;

								case "lat":

									String lats = JOptionPane.showInputDialog(self, "Please enter the Default Latitude", ""+LAT);

									if(lats!=null)
									{
										try
										{
											double latn = Double.parseDouble(lats);

											if(latn > 90 || latn < -90)
											{
												JOptionPane.showMessageDialog(self, "Needs to be between -90 and 90 degrees", "Default Latitude Out Of Range", JOptionPane.ERROR_MESSAGE);
												return;
											}

											setLAT(latn);
											SaveConfig();

										}
										catch(NumberFormatException nfe)
										{
											JOptionPane.showMessageDialog(self, "Invalid Number "+lats, "Default Latitude Out Of Range", JOptionPane.ERROR_MESSAGE);
											return;
										}

									}
									break;

								case "lon":

									String lons = JOptionPane.showInputDialog(self, "Please enter the Default Longitude", ""+LON);

									if(lons!=null)
									{
										try
										{
											double lonn = Double.parseDouble(lons);

											if(lonn > 180 || lonn < -180)
											{
												JOptionPane.showMessageDialog(self, "Needs to be between -180 and 180 degrees", "Default Latitude Out Of Range", JOptionPane.ERROR_MESSAGE);
												return;
											}

											setLON(lonn);
											SaveConfig();

										}
										catch(NumberFormatException nfe)
										{
											JOptionPane.showMessageDialog(self, "Invalid Number "+lons, "Default Longitude Out Of Range", JOptionPane.ERROR_MESSAGE);
											return;
										}

									}
									break;

								case "sync":

									if(!mon.is_online(1))
									{
										JOptionPane.showMessageDialog(self, "Not connected", "Sync Clock", JOptionPane.ERROR_MESSAGE);
										return;
									}

									try { for(i=1;i<=stations;i++) mon.sync_clock(i); }
									catch(Exception sce) { JOptionPane.showMessageDialog(self, sce.getMessage(), "Serial Exception", JOptionPane.ERROR_MESSAGE); }
									break;

								case "erase":

									if(!mon.is_online(1))
									{
										JOptionPane.showMessageDialog(self, "Not connected", "Erase Logs", JOptionPane.ERROR_MESSAGE);
										return;
									}

									if(0==JOptionPane.showConfirmDialog(self, "Press YES to erase all logged data", "Erase Logs Confirmation", JOptionPane.YES_NO_OPTION))
									{
										mon.erase_logs();
									}
									break;


								case "save":

									FileDialog fd = new FileDialog(self, "Save Logs", FileDialog.SAVE);

									Date d= new Date(System.currentTimeMillis());

									for(i=1;i<=stations;i++)
									{
										fd.setFile("results"+i+"_"+d.getDate()+"_"+(1+d.getMonth())+"_"+(1900+d.getYear())+".csv");

										fd.show();

										String fileName = fd.getFile();
										String directory = fd.getDirectory();

		        	 					if ((fileName != null) && (directory != null))
		        	 					{
											switch(i)
											{
												case 1:
													trend.save_data(directory,fileName);
													break;
												case 2:
													trend2.save_data(directory,fileName);
													break;
												case 3:
													trend3.save_data(directory,fileName);
													break;
											}
										}
									}
									break;

								case "download":

									if(!mon.is_online(1))
									{
										JOptionPane.showMessageDialog(self, "Not connected", "Download Logs", JOptionPane.ERROR_MESSAGE);
										return;
									}

									try { mon.GetLogs(1,"Test"); }
									catch(Exception sce) { JOptionPane.showMessageDialog(self, sce.getMessage(), "Serial Exception", JOptionPane.ERROR_MESSAGE); }
									break;


								case "servo_off":

									if(!mon.is_online())
									{
										JOptionPane.showMessageDialog(self, "Not connected", "Download Logs", JOptionPane.ERROR_MESSAGE);
										return;
									}

									try { for(i=1;i<=stations;i++) mon.servo(i,0); }
									catch(Exception sce) { JOptionPane.showMessageDialog(self, sce.getMessage(), "Serial Exception", JOptionPane.ERROR_MESSAGE); }
									break;

								case "servo_on":

									if(!mon.is_online())
									{
										JOptionPane.showMessageDialog(self, "Not connected", "Download Logs", JOptionPane.ERROR_MESSAGE);
										return;
									}

									try { for(i=1;i<=stations;i++) mon.servo(i,1); }
									catch(Exception sce) { JOptionPane.showMessageDialog(self, sce.getMessage(), "Serial Exception", JOptionPane.ERROR_MESSAGE); }
									break;


								case "heat":

									if(!mon.is_online())
									{
										JOptionPane.showMessageDialog(self, "Not connected", "Trigger Heater", JOptionPane.ERROR_MESSAGE);
										return;
									}

									try { mon.heat(); }
									catch(Exception sce) { JOptionPane.showMessageDialog(self, sce.getMessage(), "Serial Exception", JOptionPane.ERROR_MESSAGE); }
									break;

								case "start":


									if(!mon.is_online())
									{
										JOptionPane.showMessageDialog(self, "Not connected", "Start Fumigation Process", JOptionPane.ERROR_MESSAGE);
										return;
									}

									if(mon.is_online(1))
										announce("12");

									if(mon.is_online(2))
										announce("11");

									if(mon.is_online(3))
										announce("10");

									fumigation=true;
									fumigation_timer=0;
									light_timer=0;
									break;

								case "stop":


									if(!mon.is_online())
									{
										JOptionPane.showMessageDialog(self, "Not connected", "Stop Fumigation Process", JOptionPane.ERROR_MESSAGE);
										return;
									}


									fumigation=false;
									break;

								case "sleep":

									if(fumigation)
										fumigation_timer=7200;

									break;

								// Launch sequence
								case "launch":

									if(!mon.is_online(1))
									{
										JOptionPane.showMessageDialog(self, "Not connected", "Start Logging", JOptionPane.ERROR_MESSAGE);
										return;
									}

									Object[] possibleValues = {  "1min", "30mins", "50mins", "1hr", "2hrs", "4hrs", "6hrs","10hrs" };

									Object selectedValue = JOptionPane.showInputDialog(self,
										"Choose logging time interval", "Input",
										JOptionPane.INFORMATION_MESSAGE, null,
										possibleValues, possibleValues[0]);

									if(selectedValue==null)
										return;

									int period;

									if(selectedValue.equals("4hrs")) period=60*4;
									else if(selectedValue.equals("2hrs")) period=60*2;
									else if(selectedValue.equals("1hr")) period=60;
									else if(selectedValue.equals("30mins")) period=30;
									else if(selectedValue.equals("50mins")) period=50;
									else if(selectedValue.equals("6hrs")) period=60*6;
									else if(selectedValue.equals("10hrs")) period=60*10;
									else
										period=1;

									if(0==JOptionPane.showConfirmDialog(self, "Press YES to sleep with wakeup every "+period+" minutes?", "Sleep Mode Confirmation", JOptionPane.YES_NO_OPTION))
									{
										mon.sleep_period_update(period);
										mon.erase_logs();
									}

									break;
						}
					}

				}

			}
	}

	//
	// Fumigation process
	//
	class PollThread extends TimerTask {

		int fumigation_minute,min;
		boolean is_active,azure_active;

		synchronized public void run()
		{
			if(fumigation)
			{
				fumigation_timer ++ ;

				fumigation_minute = fumigation_timer / 60;
				//fumigation_minute = fumigation_timer;

				// first phase for 2 hrs
				if(fumigation_minute < FirstPhase*60)
				{
					is_active=true;
					azure_active=true;
				}
				else
				{
					// second phase for the next four hours.. only active for 15 mins per hour
					if(fumigation_minute <= (FirstPhase+SecondPhase)*60)
					{

						if( ( ((fumigation_minute % 60) > 45) || ((fumigation_minute % 60) == 0) ) && (fumigation_minute > FirstPhase*60+40))
						{
							is_active=true;

							// only activate azure in the last minute
							if((fumigation_minute % 60) == 0)
								azure_active=true;
							else
								azure_active=false;
						}
						else
						{
							is_active=false;
							azure_active=false;
						}
					}
					else
					{
						// third phase .. only active for 15 mins per twelve hours
						if( ((fumigation_minute % (ThirdPhase*60)) > (ThirdPhase*60)-15) || ((fumigation_minute % (ThirdPhase*60)) == 0))
						{
							is_active=true;

							// only activate azure in the last minute
							if((fumigation_minute % (ThirdPhase*60)) == 0)
								azure_active=true;
							else
								azure_active=false;
						}
						else
						{
							is_active=false;
							azure_active=false;
						}
					}
				}

				// restart if any light > 1200
				if(gsc.light1 >= 1200 || gsc.light2 >= 1200 || gsc.light3 >= 1200)
				{
					if(light_timer >= 30)
						fumigation_timer=0;
					else
						light_timer++;
				}
				else
					light_timer=0;

			}
			else
			{
				fumigation_timer=0;
				light_timer=0;
				is_active=false;
			}

			System.out.println("TIMER:"+fumigation_timer+" MINS:"+fumigation_minute+" ACTIVE:"+is_active);

			if(is_active)
			{
					// activate azure data transmission every minute
					if(gsc != null)
						gsc.active=azure_active;

					// make sure all power is on
					if(mon!=null)
					{
						for(int i=1;i<=stations;i++) mon.heater(i,1);
					}
			}
			else
			{
					// deactivate azure data transmission every minute
					if(gsc != null)
						gsc.active=false;

					// make sure all power is off
					if(mon!=null)
					{
						for(int i=1;i<=stations;i++) mon.heater(i,0);
					}

			}

			fum_status.setText(""+fumigation_timer / 3600+":"+(fumigation_timer % 3600) / 60+":"+(fumigation_timer % 60));
		}
	}

}
