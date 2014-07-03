/**
 * <p>Title: GLDebug</p>
 *
 * <p>Description:
 * </p>
 *
 * <p>Copyright: Copyright (C) 2008</p>
 *
 * <p>Company: Battelle Memorial Institute</p>
 *
 * @author Jon McCall
 * @version 1.0
 */

package gov.pnnl.gridlab.gldebug.ui;


import java.awt.BorderLayout;
import java.awt.Color;
import java.awt.Dimension;
import java.awt.Toolkit;

import javax.swing.BorderFactory;
import javax.swing.ImageIcon;
import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.JWindow;


public class SplashScreen extends JWindow
{
	private int duration;

	public SplashScreen(String filename, int d, String message)
	{
		duration = d;

		JPanel content = (JPanel)getContentPane();

		content.setBackground(Color.white);
		content.setForeground(new Color(100, 100, 100));

		ImageIcon icon = GLIcons.getImageIcon(filename);
		JLabel label = new JLabel(icon);
		label.validate();

		int width = icon.getIconWidth();
		int height = icon.getIconHeight();
		Dimension screen = Toolkit.getDefaultToolkit().getScreenSize();
		int x = (screen.width-width)/2;
		int y = (screen.height-height)/2;
		setBounds(x,y,width,height);

		JLabel messageLabel = new JLabel("  Copyright (C) 2008 Battelle Memorial Institute");

		if (message != null &&
			message.length() > 0){
				messageLabel = new JLabel(message);
			}

		validate();

		content.add(label, BorderLayout.CENTER);
		if (messageLabel != null){
			content.add(messageLabel, BorderLayout.SOUTH);
		}
		content.setBorder(BorderFactory.createLineBorder(Color.BLACK, 2));

	}

	public void showSplash()
	{

		setVisible(true);

		try { Thread.sleep(duration); } catch (Exception e) {}

		setVisible(false);
		dispose();
	  }

	  public static void showSplashScreen(String whichImg, int dur, String message)
	  {
		SplashScreen splash = new SplashScreen(whichImg, dur, message);
		splash.showSplash();
	  }
}
