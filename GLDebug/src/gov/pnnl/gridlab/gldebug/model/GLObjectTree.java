package gov.pnnl.gridlab.gldebug.model;

import java.util.ArrayList;
import java.util.List;
import java.util.TreeMap;

/**
 * maps tree object names to a list of their children.
 */
		

public class GLObjectTree extends TreeMap <String, List<GLObject>>{
	
	public static final String ROOT = "ROOT";

	/**
	 * create the tree from a list
	 * @return
	 */
	public static GLObjectTree createTree(List<GLObject> list) {
		GLObjectTree tree = new GLObjectTree();
		//GLObject root = GLObject.ROOT_OBJECT;
		//tree.put(root, new ArrayList<GLObject>());
		
		for (GLObject obj : list) {
			String name = obj.getParentName();
			List<GLObject> children = tree.get(name);
			if( children == null ) {
				children = new ArrayList<GLObject>();
				tree.put(name, children);
			}
			children.add(obj);
		}
		
		return tree;
	}

}
