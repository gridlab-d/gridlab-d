package gov.pnnl.gridlab.util;

import java.util.EventListener;

public interface ExecutionListener extends EventListener {
	public void executeEvent(ExecutionEvent evt);
}
