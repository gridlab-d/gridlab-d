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

package gov.pnnl.gridlab.gldebug.ui.util;

import java.awt.Color;

import javax.swing.JTextArea;
import javax.swing.JTextPane;
import javax.swing.text.*;

/**
 *
 */
public class StyledJTextPane extends JTextPane {

	
	
	/**
	 * 
	 */
	public StyledJTextPane() {
	    StyleContext sc = StyleContext.getDefaultStyleContext();
	    AttributeSet aset = sc.addAttribute(SimpleAttributeSet.EMPTY, StyleConstants.FontFamily, "Courier");
		
//	    StyledDocument doc = this.getStyledDocument();
//	    
//	    
//	    // Makes text red
//	    Style style = this.addStyle("Red", null);
//	    StyleConstants.setForeground(style, Color.red);
//	    
//	    // Inherits from "Red"; makes text red and underlined
//	    style = this.addStyle("Red Underline", style);
//	    StyleConstants.setUnderline(style, true);
//	    
//	    // Makes text 12pts
//	    style = this.addStyle("12pts", null);
//	    StyleConstants.setFontSize(style, 12);
//	    
//	    // Makes text italicized
//	    style = this.addStyle("Italic", null);
//	    StyleConstants.setItalic(style, true);
//	    
//	    // A style can have multiple attributes; this one makes text bold and italic
//	    style = this.addStyle("Bold Italic", null);
//	    StyleConstants.setBold(style, true);
//	    StyleConstants.setItalic(style, true);
//	 
	    
/*	 // Set text in the range [5, 7) red
	    doc.setCharacterAttributes(5, 2, textPane.getStyle("Red"), true);
	    
	    // Italicize the entire paragraph containing the position 12
	    doc.setParagraphAttributes(12, 1, textPane.getStyle("Italic"), true);

*/	    
	}
	
	public void append(String msg) {
	    int len = getDocument().getLength(); // same value as
	    try {
		    getDocument().insertString(len, msg, null);
		} catch (Exception e) {
			e.printStackTrace();
		}
		// this old way is very slow compared to above...
//	    setCaretPosition(len); // place caret at the end (with no selection)
//	    setEditable(true);
//	    replaceSelection(msg); // there is no selection, so inserts at caret
//	    setEditable(false);
	}

	public void appendStyled(String msg, Color c) {
	    StyleContext sc = StyleContext.getDefaultStyleContext();
	    AttributeSet aset = sc.addAttribute(SimpleAttributeSet.EMPTY, StyleConstants.Foreground, c);

	    int len = getDocument().getLength(); 
	    setCaretPosition(len); // place caret at the end (with no selection)
	    setCharacterAttributes(aset, false);
	    setEditable(true);
	    replaceSelection(msg); // there is no selection, so inserts at caret
	    setEditable(false);

	    // put it back to black
	    aset = sc.addAttribute(SimpleAttributeSet.EMPTY, StyleConstants.Foreground, Color.black);
	    setCharacterAttributes(aset, false);
	}
	

}
