package gov.pnnl.gridlab.gldebug.ui.util;

/**
 * <p>Title: </p>
 * <p>Description: </p>
 * <p>Copyright: Copyright (c) 2003</p>
 * <p>Company: Battelle Memorial Institute</p>
 * @author unascribed
 * @version 1.0
 */

import java.util.*;
import java.text.*;
import java.awt.*;
import java.awt.geom.*;

public class Utils {

    /**
     * put your documentation comment here
     * @param val
     * @param minChars
     * @return
     */
    public static String stringRep (int val, int minChars) {
        int abs = Math.abs(val);
        double width = 1.0;
        double v = abs;
        while (v >= Math.pow(10.0, width)) {
            width += 1.0;
        }
        StringBuffer sb = new StringBuffer();
        if (val < 0)
            sb.append('-');
        int n = minChars - (int)width;
        for (int i = 0; i < n; i++) {
            sb.append("0");
        }
        sb.append(String.valueOf(abs));
        return  sb.toString();
    }


    /**
     * put your documentation comment here
     * @param time
     * @return
     */
    public static String timeStamp (long time) {
        Calendar cal = Calendar.getInstance();
        cal.setTime(new Date(time));
        return  stringRep(cal.get(Calendar.MONTH) + 1, 2) + '/' + stringRep(cal.get(Calendar.DAY_OF_MONTH),
                2) + '/' + cal.get(Calendar.YEAR) + ' ' +
                stringRep(cal.get(Calendar.HOUR_OF_DAY), 2) + ':' + stringRep(cal.get(Calendar.MINUTE),
                2) + ':' + stringRep(cal.get(Calendar.SECOND), 2);
    }


    /**
     * Position a window at the location specified.<p>
     *
     * If the pos is not on the screen, the window is moved so
     * that it will be visible.  This takes into account multiple
     * monitors.
     */
    public static void positionWindow (Window win, Point pos, Dimension size) {
         Rectangle screen = getDesktopBounds();

         if( size.height > screen.height )
             size.height = screen.height;

         if( size.width > screen.width )
             size.width = screen.width;

         if( pos.x < screen.x )
             pos.x = screen.x;
         if( pos.x + size.width  > screen.x + screen.width )
             pos.x = screen.x + screen.width - size.width;

         if( pos.y < screen.y )
             pos.y = screen.y;
         if( pos.y + size.height > screen.y + screen.height )
             pos.y = screen.y + screen.height - size.height;

         win.setLocation(pos);
         win.setSize(size);
    }

    public static void positionWindow(Window win, Rectangle rect) {
        positionWindow(win, new Point(rect.x, rect.y), new Dimension(rect.width, rect.height));
    }

    /**
     * get the bounds of the users desktop... including if it uses multiple monitors.
     * See the Documentation on Class GraphicsConfiguration.
     */
    public static Rectangle getDesktopBounds () {
         Rectangle virtualBounds = new Rectangle();
         GraphicsEnvironment ge = GraphicsEnvironment.getLocalGraphicsEnvironment();
         GraphicsDevice[] gs = ge.getScreenDevices();
         for (int j = 0; j < gs.length; j++) {
            GraphicsDevice gd = gs[j];
            GraphicsConfiguration gc = gd.getDefaultConfiguration();
            Rectangle rect = gc.getBounds();
            if( rect != null )
                virtualBounds = virtualBounds.union(rect);

//            GraphicsConfiguration[] gc =gd.getConfigurations();
//            for (int i=0; i  < gc.length; i++) {
//                 Rectangle rect = gc[i].getBounds();
//                 if( rect != null )
//                     virtualBounds = virtualBounds.union(rect);
//            }
         }
         return virtualBounds;
    }

    /**
     * given the location on screen, get the screen device bounds containing this point.
     * For multiple monitors, we want to return the size of the monitor containing the point.
     * Also, since the positioning might be slightly outside, we get the rect closest to in that case.
     */
    public static Rectangle getScreenBoundsByPoint(Point loc) {
        GraphicsEnvironment ge = GraphicsEnvironment.getLocalGraphicsEnvironment();
        // maxBounds below removes the windows taskbar from the equation
        Rectangle maxBounds = ge.getMaximumWindowBounds();
        GraphicsDevice[] gs = ge.getScreenDevices();
        double closest = Double.MAX_VALUE;
        Rectangle closestRect = null;

        for (int j = 0; j < gs.length; j++) {
           GraphicsDevice gd = gs[j];
           GraphicsConfiguration gc = gd.getDefaultConfiguration();
           Rectangle rect = gc.getBounds();
           if( rect != null )
               if( rect.contains(loc) ) {
                   closestRect = rect;
                   break;
               }
               double d = loc.distance(rect.getLocation());
               if( d < closest ) {
                   closest =d;
                   closestRect = rect;
               }
        }
        // if screen size is bigger then the max bounds (which does not include windows taskbar), then return smaller size
        if( closestRect.width > maxBounds.width || closestRect.height > maxBounds.height ) {
            return maxBounds;
        }
        return closestRect;
    }
    
    /**
	 * returns the upper right screen coordinate of the screen farthest to the right and top
	 * that the given window covers.  
	 */
	public static Point getUpperRightScreenCoord(java.awt.Window win) {
		Point screen = win.getLocationOnScreen();
		Rectangle rect = win.getBounds();
		Rectangle screenRect = getScreenBoundsByPoint(new Point(screen.x + rect.width, screen.y));
		
		return new Point(screenRect.x + screenRect.width, screenRect.y);
	}


    /**
     *
     */
    public static void centerOverDesktop (java.awt.Window win){
        Dimension dim = Toolkit.getDefaultToolkit().getScreenSize();
        centerOver(win, dim.width/2, dim.height/2);
    }


    public static void centerOver(java.awt.Window winToShow, java.awt.Window winBelow) {
       Point p = winBelow.getLocation();
       // if winBelow is minimized it's position is greater than 32k
       if( p.x > 32000 || p.y > 32000 ) {
           centerOverDesktop(winToShow);
           return;
       }

       Dimension d = winBelow.getSize();
       int x = p.x + d.width/2;
       int y = p.y + d.height/2;
       centerOver(winToShow, x, y);
    }

    public static void centerOver(java.awt.Window win, int x, int y) {
      Dimension winSize = win.getSize();
      if(winSize.width == 0) {
        win.pack();
        winSize = win.getSize();
      }

      // get the screen bounds for the screen we are displaying on and
      // make sure the width/height is not bigger than the screen
      Rectangle maxRect = getScreenBoundsByPoint(new Point(x, y));
      boolean adjustedSize = false;
      if( winSize.width > maxRect.width ) {
          winSize.width = maxRect.width;
          adjustedSize = true;
      }
      if( winSize.height > maxRect.height ) {
          winSize.height = maxRect.height;
          adjustedSize = true;
      }
      int x2 = Math.max(0, x - winSize.width/2);
      int y2 = Math.max(0, y - winSize.height/2);
      win.setLocation(x2, y2);
      if( adjustedSize ) {
          win.setSize(winSize.width, winSize.height);
      }
    }

    /**
     * convert a time to the corresponding time of the start and end of
     * the day it exists on.
     *
     * @param time
     * @return
     */
    public static long[] converTimeToDayStartEnd(long time, long[]out) {
        Calendar cal = new GregorianCalendar();
        Calendar calEnd = new GregorianCalendar();
        cal.setTime(new Date(time));
        cal.set(cal.get(Calendar.YEAR), cal.get(Calendar.MONTH), cal.get(Calendar.DAY_OF_MONTH), 0, 0, 0);
        calEnd.setTime(cal.getTime());
        calEnd.add(Calendar.DATE, 1); // add one day to end date

        if( out == null ) {
            return new long[] {cal.getTimeInMillis(), calEnd.getTimeInMillis()};
        }
        else {
            out[0] = cal.getTimeInMillis();
            out[1] = calEnd.getTimeInMillis();
            return out;
        }

    }

    /**
     * put your documentation comment here
     * @return
     */
    public static String timeStamp () {
        return  timeStamp(System.currentTimeMillis());
    }


    /**
     * put your documentation comment here
     * @param time
     * @return
     */
    public static String elapsedTimeStamp (long time) {
        StringBuffer sb = new StringBuffer();
        elapsedTimeStampSection(time/86400000L, "day", sb);
        elapsedTimeStampSection((time%86400000L)/3600000L, "hour", sb);
        elapsedTimeStampSection((time%3600000L)/1000L, "second", sb);
        return  sb.toString();
    }

    /**
     * put your documentation comment here
     * @param n
     * @param label
     * @param sb
     */
    private static void elapsedTimeStampSection (long n, String label, StringBuffer sb) {
        if (n > 0) {
            if (sb.length() > 0)
                sb.append(", ");
            sb.append(n);
            sb.append(' ');
            sb.append(label);
            if (n > 1)
                sb.append('s');
        }
    }

    /**
     * chop a wide string into multiple portions, each no longer than maxColWidth,
     * so that you can display the string a line at a time.
     *
     */
    public static java.util.List<String> chopString (String str, int maxColWidth){
        java.util.List<String> items = new ArrayList<String>();

        int index = 0;
        while( index + maxColWidth < str.length() ) {
            int x = str.lastIndexOf(" ", index + maxColWidth);
            if (x < index) {
                items.add(str.substring(index, index + maxColWidth));
                index += maxColWidth;
            }
            else {
                items.add(str.substring(index, x));
                index = x;
            }
        }

        // add last bit
        items.add(str.substring(index));

        return items;
    }

    /**
     * convert a time value to a nice to look at string, "HH:mm:ss"
     */
    public static String getElapsedTimeString(long time) {
        double totalSeconds = time / 1000d;
        double totalMin = totalSeconds / 60;
        double totalHours = totalMin / 60;
        int hours = (int)Math.floor(totalHours);
        int min = (int)(Math.floor(totalMin) - (hours * 60));
        int seconds = (int)(Math.floor(totalSeconds) - (hours * 3600) - (min * 60));
        StringBuffer sbuf = new StringBuffer();
        sbuf.append(hours);
        sbuf.append(":");
        if( min < 10 ) {
            sbuf.append("0");
        }
        sbuf.append(min);
        sbuf.append(":");

        if( seconds < 10 ) {
            sbuf.append("0");
        }
        sbuf.append(seconds);

        return sbuf.toString();
    }

}
