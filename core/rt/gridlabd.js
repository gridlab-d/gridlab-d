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
	    update(document.location + "?target=" + obj.name + "&set=" + obj.value + "");
	else
	    update(document.location + "?target=" + obj.name + "&clear=" + obj.value + "");
}
function click_action(obj)
{
    update(document.location + "?" + obj.name + "=" + obj.value + "");
}
function GLDGetGlobal(name)
{
    xml = new XMLHttpRequest();
    xml.open("GET","/"+name,false);
    xml.send();
    doc = xml.responseXML;
    var x = doc.getElementsByTagName("globalvar")
    return x[0].getElementsByTagName("value")[0].childNodes[0].nodeValue;
}
function GLDSetGlobal(name,value)
{
    xml = new XMLHttpRequest();
    xml.open("GET","/"+name+"="+value,false);
    xml.send();
    doc = xml.responseXML;
    var x = doc.getElementsByTagName("globalvar")
    return x[0].getElementsByTagName("value")[0].childNodes[0].nodeValue;
}
function GLDGetProperty(object,name)
{
    xml = new XMLHttpRequest();
    xml.open("GET","/"+object+"/"+name,false);
    xml.send();
    doc = xml.responseXML;
    var x = doc.getElementsByTagName("property")
    return x[0].getElementsByTagName("value")[0].childNodes[0].nodeValue;
}
function GLDSetProperty(object,name,value)
{
    xml = new XMLHttpRequest();
    xml.open("GET","/"+object+"/"+name+"="+value,false);
    xml.send();
    doc = xml.responseXML;
    var x = doc.getElementsByTagName("property")
    return x[0].getElementsByTagName("value")[0].childNodes[0].nodeValue;
}

