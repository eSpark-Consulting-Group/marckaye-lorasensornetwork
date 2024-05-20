
/*
	Dynamic Data Series

*/

import java.awt.BorderLayout;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.Color;
import javax.swing.*;
import java.awt.event.*;
import org.jfree.chart.ChartFactory;
import org.jfree.chart.ChartPanel;
import org.jfree.chart.JFreeChart;
import org.jfree.chart.axis.ValueAxis;
import org.jfree.chart.plot.XYPlot;
import org.jfree.data.time.Second;
import org.jfree.data.time.TimeSeries;
import org.jfree.data.time.TimeSeriesDataItem;
import org.jfree.data.time.TimeSeriesCollection;
import org.jfree.data.time.RegularTimePeriod;
import org.jfree.data.xy.XYDataset;
import org.jfree.ui.ApplicationFrame;
import org.jfree.ui.RefineryUtilities;
import java.awt.Component;
import java.io.*;

public class TrendChart {

    /** The time series data. */
    private TimeSeries series[];
	private JFreeChart chart;

	private final int channels=14;

	private final String chname[] = {
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
			"Voc2" };


    /**
     * Constructs a new demonstration application.
     *
     * @param title  the frame title.
     */
    public TrendChart(int xs, int ys, final String title, JPanel content) {


		int i;

        this.series = new TimeSeries[channels];

        final TimeSeriesCollection dataset = new TimeSeriesCollection();

		for(i=0;i<channels;i++)
		{
			series[i] = new TimeSeries(chname[i] , Second.class);
			dataset.addSeries(series[i]);

		}

        chart = createChart(title,dataset);

        final ChartPanel chartPanel = new ChartPanel(chart);

        content.add(chartPanel);
        chartPanel.setPreferredSize(new java.awt.Dimension(xs, ys));
    }

	public void set_time_axis(int channel, double time_ms)
	{
		XYPlot plot = chart.getXYPlot();

     	ValueAxis axis = plot.getDomainAxis();

        axis.setFixedAutoRange(time_ms);

		channel--;

		if((channel < channels) && (channel >= 0))
		{
			this.series[channel].clear();

			Second now = new Second();

			RegularTimePeriod tp = (RegularTimePeriod)now;

			for(double t=0;t < time_ms; t+= 1000)
			{

				tp = tp.next();

				this.series[channel].addOrUpdate(tp, 0);

			}
		}

	}

	public void add_data(int channel,double value)
	{

			Second now = new Second();

			channel--;

			if((channel < channels) && (channel >= 0))
			{
				this.series[channel].addOrUpdate(now, value);
			}
	}

	public void save_data(String directory,String fileName)
	{
		FileOutputStream fileOut=null;

								try {

								 	fileOut = new FileOutputStream(directory + fileName);

									String header="Time,light,temp,humid,Voc,Nox,O2,NDIR,S302,S402,S502,S602,S702,S802,Voc2\n";

								 	fileOut.write(header.getBytes());

								 	for(int index=0;index<this.series[0].getItemCount();index++)
								 	{
										String line="";

										for(int ch=0;ch<channels;ch++)
										{
											TimeSeriesDataItem d = this.series[ch].getDataItem(index);

											if(ch == 0)
												line = ""+d.getPeriod();

											line += ","+d.getValue();
										}

										line += "\n";

  									 fileOut.write(line.getBytes());

									}

								 	fileOut.close();
								 } catch (IOException e2)
								 {

									try { fileOut.close(); } catch(IOException e3) { }
								 }

	}


    /**
     * Creates a sample chart.
     *
     * @param dataset  the dataset.
     *
     * @return A sample chart.
     */
    private JFreeChart createChart(final String title, final XYDataset dataset) {
        final JFreeChart result = ChartFactory.createTimeSeriesChart(
            "",
            "Time",
            "",
            dataset,
            true,
            true,
            false
        );
        final XYPlot plot = result.getXYPlot();
        ValueAxis axis = plot.getDomainAxis();
        axis.setAutoRange(true);
        axis.setFixedAutoRange(5*60000.0);  // 5*60 seconds
        axis = plot.getRangeAxis();
        axis.setRange(0, 65535);
        return result;
    }

}