
/*
	Progress bar window class
*/


import javax.swing.*;
import java.awt.*;
import java.awt.event.*;

public class Progress extends JFrame
{
	private JProgressBar progressBar;
	private boolean closed;

	Progress(JFrame parent, int min,int max)
	{
		super("Progress");

		setLocationRelativeTo(parent);
		setSize(300,70);

        addWindowListener(new CloseHandler(this));

		closed=false;

			progressBar = new JProgressBar(min, max);
			progressBar.setValue(min);
			progressBar.setStringPainted(true);
			add(progressBar);
			setVisible(true);
			Thread.yield();
	}

	public void close()
	{
		closed=true;
		progressBar.setVisible(false);
		remove(progressBar);
		setVisible(false);
	}

	public boolean setValue(int x)
	{
		progressBar.setValue(x);

		Thread.yield();

		return !closed;
	}

            /**
            Handles closing down system. Allows application to be closed with window
            close box.
            */
            class CloseHandler extends WindowAdapter {

            Progress pb;

            public CloseHandler(Progress pb) {
                this.pb = pb;
            }

            public void windowClosing(WindowEvent e) {
                pb.close();

            }
        }
}
