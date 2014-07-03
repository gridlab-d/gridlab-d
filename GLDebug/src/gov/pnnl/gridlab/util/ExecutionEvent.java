package gov.pnnl.gridlab.util;

public class ExecutionEvent extends java.util.EventObject {

	public static final int STDOUT    = 0;
  public static final int STDERR    = 1;
	public static final int FINISHED  = 2;
	public static final int HALTED    = 3;
  public static final int ERROR     = 4;

  private String message;
  private int messageType;

	public ExecutionEvent(Object source, String message_in, int typein) {
		super(source);
		message = message_in;
		messageType = typein;
	}

  public String getMessage() { return message; }
  public int getMessageType() { return messageType; }
}
