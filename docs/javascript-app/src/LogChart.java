
/*
	JFreechart to view all logged data

*/

import java.awt.*;
import java.awt.event.*;
import java.text.SimpleDateFormat;
import java.text.FieldPosition;
import java.util.Date;
import javax.swing.*;
import javax.swing.table.*;
import org.jfree.chart.*;
import org.jfree.chart.axis.NumberAxis;
import org.jfree.chart.axis.ValueAxis;
import org.jfree.chart.event.*;
import org.jfree.chart.plot.PlotOrientation;
import org.jfree.chart.plot.XYPlot;
import org.jfree.chart.renderer.xy.XYItemRenderer;
import org.jfree.chart.renderer.xy.XYLineAndShapeRenderer;
import org.jfree.data.Range;
import org.jfree.data.RangeType;
import org.jfree.data.time.*;
import org.jfree.data.xy.XYDataset;
import org.jfree.ui.*;
import java.io.FileOutputStream;
import java.io.IOException;

public class LogChart extends ApplicationFrame implements ActionListener
{
	Log _events[];
	int _no_events;
	String _title;
	Frame _parent;

    private static class DemoPanel extends JPanel
    {
		JButton saveButton;
		JButton closeButton;

        private XYDataset createDataset(int i, String s, Log events[], int no_events, int chanID)
        {
			int j,k;
			Date d;

            series[i] = new TimeSeries(s, new Minute().getClass());

			if(no_events > 0)
			{
			 // add data here
			 for(j=no_events-1;j>=0;j--)
			 {

						d = events[j].getChannelTimestamp();

						series[i].addOrUpdate(new Minute(d),
														events[j].getChannelValue(chanID));

			 }
			}

            datasets[i] = new TimeSeriesCollection();
            datasets[i].addSeries(series[i]);
            return datasets[i];
        }

        private JFreeChart createChart(String title, Log events[], int no_events)
        {
            JFreeChart jfreechart = ChartFactory.createTimeSeriesChart(title, "Time", events[0].getChannelUnit(0), null, true, true, false);
            XYDataset axydataset[] = new XYDataset[events[0].getChannelCount()];
            int axes_assigned[] = new int[events[0].getUnitCount()];
			XYPlot xyplot=jfreechart.getXYPlot();
			RegularTimePeriod min;
			int i;

			// create the axes
			for(i=0;i<events[0].getUnitCount();i++)
				axes_assigned[i] = -1;

			// create the datasets
			for(i=0;i<events[0].getChannelCount();i++)
			{
				axydataset[i] = createDataset(i, events[0].getChannelName(i), events, no_events, i);

				String units = events[0].getChannelUnit(events[0].getChannelUnitIndex(i));

				NumberAxis na = new NumberAxis(units);

			 	na.setRangeType(RangeType.POSITIVE);

				na.setAutoRangeIncludesZero(true);

				if(i > 0)
				{
				      xyplot.setDataset(i, axydataset[i]);

					 if(axes_assigned[events[0].getChannelUnitIndex(i)] < 0)
					 {
				      	 xyplot.setRangeAxis(i, na);
				      	 axes_assigned[events[0].getChannelUnitIndex(i)]=i;

	                  	xyplot.mapDatasetToRangeAxis(i, i);
					}
					else
					{

	                  xyplot.mapDatasetToRangeAxis(i, axes_assigned[events[0].getChannelUnitIndex(i)]);
					}

				      xyplot.setRenderer(i, new XYLineAndShapeRenderer(true, false));

			    }
				else // Vdc
				{
					xyplot.setDataset(axydataset[i]);
					xyplot.setRangeAxis(i, na);
			      	axes_assigned[events[0].getChannelUnitIndex(i)]=i;
				}


			}

            jfreechart.setBackgroundPaint(Color.white);

            xyplot.setOrientation(PlotOrientation.VERTICAL);
            xyplot.setBackgroundPaint(Color.lightGray);
            xyplot.setDomainGridlinePaint(Color.white);
            xyplot.setRangeGridlinePaint(Color.white);
            xyplot.setAxisOffset(new RectangleInsets(5D, 5D, 5D, 5D));
            xyplot.setDomainCrosshairVisible(true);
            xyplot.setDomainCrosshairLockedOnData(false);
            xyplot.setRangeCrosshairVisible(false);
            XYItemRenderer xyitemrenderer = xyplot.getRenderer();
            xyitemrenderer.setPaint(Color.black);
            return jfreechart;
        }

        private TimeSeriesCollection datasets[];
        private TimeSeries series[];
        private ChartPanel chartPanel;

        public DemoPanel(String title, Log events[], int no_events)
        {
            super(new BorderLayout());
            datasets = new TimeSeriesCollection[events[0].getChannelCount()];
            series = new TimeSeries[events[0].getChannelCount()];
            JPanel jpanel = new JPanel(new BorderLayout());
            JFreeChart jfreechart = createChart(title, events, no_events);
            chartPanel = new ChartPanel(jfreechart);
            chartPanel.setPreferredSize(new Dimension(800, 300));
            chartPanel.setDomainZoomable(true);
            chartPanel.setRangeZoomable(true);
            javax.swing.border.CompoundBorder compoundborder = BorderFactory.createCompoundBorder(BorderFactory.createEmptyBorder(4, 4, 4, 4), BorderFactory.createEtchedBorder());
            chartPanel.setBorder(compoundborder);
            jpanel.add(chartPanel);
            JPanel jpanel1 = new JPanel(new GridLayout(1,4));

           // jpanel1.setBorder(BorderFactory.createEmptyBorder(0, 4, 4, 4));

			JLabel spc1 = new JLabel("");
			jpanel1.add(spc1);

        	saveButton = new JButton("Save");
        	jpanel1.add(saveButton);

        	closeButton = new JButton("Close");
        	jpanel1.add(closeButton);

			JLabel spc2 = new JLabel("");
			jpanel1.add(spc2);

            jpanel.add(jpanel1, "South");
            add(jpanel);

        }
    }

	/*
      Handles events from the buttons.
    */
    public void actionPerformed(ActionEvent e) {
        String cmd = e.getActionCommand();
		int i,j;

        if(cmd.equals("Close"))
        {
            setVisible(false);
            dispose();

        }

        if(cmd.equals("Save") && _no_events > 0)
        {

       				FileDialog fd = new FileDialog(this, "Save Log",
                           FileDialog.SAVE);

					Date d= new Date(System.currentTimeMillis());

					fd.setFile(_title+"_"+d.getDate()+"_"+(1+d.getMonth())+"_"+(1900+d.getYear())+".csv");

        			fd.show();

        			String fileName = fd.getFile();
        			String directory = fd.getDirectory();

        	 		if ((fileName != null) && (directory != null))
        	 		{
                		 try
                		{
                   		 FileOutputStream fileOut = new FileOutputStream(directory + fileName);

						 String header="Timestamp";

						 for(i=0;i<Log.getChannelCount();i++)
						 {
								header += ","+_events[0].getChannelName(i) + " (" + _events[0].getChannelUnit(_events[0].getChannelUnitIndex(i)) + ")";
						 }
						 header += "\n";

						 fileOut.write(header.getBytes());

						SimpleDateFormat df = new SimpleDateFormat("HH:mm:ss dd/MM/yyyy");

						df.setLenient(true);

						FieldPosition fp=new FieldPosition(SimpleDateFormat.DEFAULT);

 							 for(i=0;i<_no_events;i++)
							 {

								 StringBuffer sb = new StringBuffer(32);

								 df.format(_events[i].getChannelTimestamp(), sb, fp);

								 String line = sb.toString();

								 for(j=0;j<Log.getChannelCount();j++)
								 {

									line += ","+_events[i].getChannelValue(j);

								  }

								line += "\n";
								fileOut.write(line.getBytes());

							 }


                   		 fileOut.close();
                		}
                		catch (IOException e2)
                		{

                		}
					}

        }
    }

    public void windowClosing(WindowEvent event)
    {
        if(event.getWindow() == this)
        {
            setVisible(false);
            dispose();

        }
    }


    public LogChart(Frame parent, String title, Log events[], int no_events)
    {

		super(title);

		DemoPanel dp;

		_title = title;
		_events = events;
		_no_events = no_events;
		_parent = parent;

			setIconImage(new ImageIcon("icon.gif").getImage());

		dp = new DemoPanel(title, events, no_events);

        setContentPane(dp);

        dp.saveButton.addActionListener(this);
        dp.closeButton.addActionListener(this);

		//setLocation(0,0);

		setLocationByPlatform(true);

    }


}