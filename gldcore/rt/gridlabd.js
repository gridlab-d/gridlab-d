// dynamic GUI update functions
function update(str)
{
    window.status = str;
}
function update_char8(obj)
{
	update(document.location + "?target=" + obj.name + "&value=" + obj.value + "");
}
function update_char32(obj)
{
	update(document.location + "?target=" + obj.name + "&value=" + obj.value + "");
}
function update_char256(obj)
{
	update(document.location + "?target=" + obj.name + "&value=" + obj.value + "");
}
function update_char1024(obj)
{
	update(document.location + "?target=" + obj.name + "&value=" + obj.value + "");
}
function update_int8(obj)
{
	update(document.location + "?target=" + obj.name + "&value=" + obj.value + "");
}
function update_int16(obj)
{
	update(document.location + "?target=" + obj.name + "&value=" + obj.value + "");
}
function update_int32(obj)
{
	update(document.location + "?target=" + obj.name + "&value=" + obj.value + "");
}
function update_int64(obj)
{
	update(document.location + "?target=" + obj.name + "&value=" + obj.value + "");
}
function update_double(obj)
{
	update(document.location + "?target=" + obj.name + "&value=" + obj.value + "");
}
function update_complex(obj)
{
	update(document.location + "?target=" + obj.name + "&value=" + obj.value + "");
}
function update_enumeration(obj)
{
	update(document.location + "?target=" + obj.name + "&value=" + obj.value + "");
}
function update_set(obj)
{
    if (obj.checked) 
	{
		update(document.location + "?target=" + obj.name + "&set=" + obj.value + "");
	}
	else 
	{
		update(document.location + "?target=" + obj.name + "&clear=" + obj.value + "");
	}
}
function click_action(obj)
{
    update(document.location + "?" + obj.name + "=" + obj.value + "");
}

var hostname = "localhost";
var portnum = 6267;
var mls = null;
var monitor = null;
var monitor_time = 5000;
var callback = null;
function GLDConnect(server,refresh_time)
{
    specs = server.split(":");
    hostname = specs[0];
    portnum = specs[1];
    mls = GLDGetGlobal("mainloop_state");
    if ( monitor )
        clearTimeout(monitor);
    monitor = null;
    mls = GLDGetGlobal("mainloop_state");
    if ( mls!=null )
    {
        monitor_time = refresh_time;
        if ( monitor_time>0 )
            monitor = setTimeout(GLDRefreshServerStatus,monitor_time);
        else
            monitor = null;
    }
    return mls;
}
/// Set the heartbeat function
/// @returns Callback position 0
function GLDSetRefreshCallback(call) ///< the function to call as a string
{
    callback = call ;
    return 0;
}
/// Add a heartbeat function to the existing list of functions
/// @returns Position of the function in the callback list
function GLDAddRefreshCallback(call) ///< the function to call as a string
{
    pos = callback.strlen();
    callback += call;
    return pos;
}
/// Delete a heartbeat function from the callback list
/// @returns 1 on success, 0 on failure
function GLDDelRefreshCallback(pos) ///< the position of the function when added (-1 deletes all)
{
    alert("GLDDelRefreshCallback not implemented yet");
}
function GLDRefreshServerStatus()
{
    mls = GLDGetGlobal("mainloop_state");
    if ( callback )
        eval(callback);
    monitor = setTimeout(GLDRefreshServerStatus,monitor_time);
}
function GLDServerStatus()
{
    if ( monitor==null )
        mls = GLDGetGlobal("mainloop_state");
    return mls;
}
function GLDGetDialogServer(dlg)
{
    return dlg.dialogArguments.server;
}
function GLDGetDialogArgs(dlg)
{
    return dlg.dialogArguments.args;
}
function GLDDialog(name,args,features)
{
    if ( mls!=null )
    {
        server = new Object();
        server.hostname = hostname;
        server.portnum = portnum;
    }
    else
        server = null;
    dlgargs = new Object();
    dlgargs.server = server;
    dlgargs.args = args;
    if (features==null)
        features = "status:1";
    window.showModalDialog(name,dlgargs,features);
}

// GLD data access functions
function XMLSend(request)
{
    if ( hostname==null || portnum==null )
        return null;
    if ( window.XMLHttpRequest )
        xml = new XMLHttpRequest();
    else if ( window.ActiveXObject )
        xml = new ActiveXObject("Microsoft.XMLHTTP");
    xml.open("GET",request,false);
    xml.send();
    return xml;
}
function XMLGet(request)
{
    xml = XMLSend(request);
    if ( xml )
        return xml.responseXML;
    else
        return null;
}
function GLDControl(name)
{
    XMLSend("http://"+hostname+":"+portnum+"/control/"+name);
}
function GLDGetValue(doc,tag)
{
    if (doc) {
		x = doc.getElementsByTagName(tag);
		if ( x.length>0 )
    	{
    	    n = x[0].getElementsByTagName("value");
    	    if ( n.length>0)
    	    {
    	        v = n[0].childNodes;
    	        if ( v.length>0 )
    	            return v[0].nodeValue;
    	    }
    	}
	}
	return null;
}
function GLDGetGlobal(name)
{
    doc = XMLGet("http://"+hostname+":"+portnum+"/xml/"+name);
    return GLDGetValue(doc,"globalvar");
}
function GLDSetGlobal(name,value)
{
    doc = XMLGet("http://"+hostname+":"+portnum+"/xml/"+name+"="+value);
    return GLDGetValue(doc,"globalvar");
}
function GLDGetProperty(object,name)
{
    doc = XMLGet("http://"+hostname+":"+portnum+"/xml/"+object+"/"+name);
    return GLDGetValue(doc,"property");
}
function GLDSetProperty(object, name, value){
    doc = XMLGet("http://"+hostname+":"+portnum+"/xml/"+object+"/"+name + "=" + value);
    return GLDGetValue(doc,"property");
}
function GLDAction(name)
{
    XMLSend("http://"+hostname+":"+portnum+"/action/"+name);
}
function GLDOpen(file)
{
    XMLSend("http://"+hostname+":"+portnum+"/open/"+file);
}

// version info
function GLDVersion(tag)
{
    var version;
    var major;
    var minor;
    var patch;
    var build;
    var branch;
    if ( !version && hostname!=null && portnum!=null )
    {
        major = GLDGetGlobal("version.major");
        minor = GLDGetGlobal("version.minor");
        patch = GLDGetGlobal("version.patch");
        build = GLDGetGlobal("version.build");
        branch = GLDGetGlobal("version.branch");
        version = major + "." + minor + "." + patch + "-" + build + " (" + branch + ")";
    }
    else
    {
        major = 3;
        minor = 0;
        patch = 0;
        build = 0;
        branch = "Grizzly";
    }
    if (tag=="major")
        return major;
    else if (tag=="minor")
        return minor;
    else if (tag=="patch")
        return patch;
    else if (tag=="build")
        return build;
    else if (tag=="branch")
        return branch;
    else
        return version;
}

// standalone GUI update functions
function GUIsave(name,variable)
{
    GLDSetGlobal(variable,document.getElementById(name).value);
    disable(name);
}
function GUIreset(name,variable)
{
    document.getElementById(name).value = GLDGetGlobal(variable);
    disable(name);
}
function GUIenable(name)
{
    document.getElementById(name+"_ok").disabled = false;
    document.getElementById(name+"_undo").disabled = false;
}
function GUIdisable(name)
{
    document.getElementById(name+"_ok").disabled = true;
    document.getElementById(name+"_undo").disabled = true;
}
function GUIhelp(name)
{
    alert("No help available for '"+name+"'!");
}
function GUInext()
{
    document.location = "/action/continue";
}

// gui entities
function GUIinput(name,specs)
{
    document.writeln('<input id="'+name+'" value="" onfocus="GUIenable(\''+name+'\');" />');
    document.writeln('<input type="submit" name="'+name+'_ok" value="&radic;" disabled onclick="GUIsave(\''+name+'\',\''+name+'\');" />');
    document.writeln('<input type="submit" name="'+name+'_undo" value="X" disabled onclick="GUIreset(\''+name+'\',\''+name+'\');" />');
    document.writeln('<input type="submit" name="'+name+'_undo" value="?" onclick="GUIhelp(\''+name+'\');" />');
}