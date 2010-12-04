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

// GLD data access functions
function GLDGetGlobal(name)
{
    xml = new XMLHttpRequest();
    xml.open("GET","/xml/"+name,false);
    xml.send();
    doc = xml.responseXML;
    if (doc) {
		var x = doc.getElementsByTagName("globalvar");
		return x[0].getElementsByTagName("value")[0].childNodes[0].nodeValue;
	}
	else 
	{
		return "(na)";
	}
}
function GLDSetGlobal(name,value)
{
    xml = new XMLHttpRequest();
    xml.open("GET","/xml/"+name+"="+value,false);
    xml.send();
    doc = xml.responseXML;
    if (doc) {
		var x = doc.getElementsByTagName("globalvar");
		return x[0].getElementsByTagName("value")[0].childNodes[0].nodeValue;
	}
	else 
	{
		return "(na)";
	}
}
function GLDGetProperty(object,name)
{
    xml = new XMLHttpRequest();
    xml.open("GET","/xml/"+object+"/"+name,false);
    xml.send();
    doc = xml.responseXML;
    if (doc) {
		var x = doc.getElementsByTagName("property");
		return x[0].getElementsByTagName("value")[0].childNodes[0].nodeValue;
	}
	else {
		return "(na)";
	}
}
function GLDSetProperty(object, name, value){
	xml = new XMLHttpRequest();
	xml.open("GET", "/xml/" + object + "/" + name + "=" + value, false);
	xml.send();
	doc = xml.responseXML;
	if (doc) {
		var x = doc.getElementsByTagName("property");
		return x[0].getElementsByTagName("value")[0].childNodes[0].nodeValue;
	}
	else {
		return "(na)";
	}
}
function GLDAction(name)
{
    xml = new XMLHttpRequest();
    xml.open("GET","/action/"+name,false);
    xml.send();
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