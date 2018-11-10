<!-- index.php
     Copyright (C) 2016, Stanford University
     Author: dchassin@slac.stanford.edu
--><HTML>
<PRE>
<?php
	date_default_timezone_set(UTC);
	include("config/default.php");
	function error($msg)
	{
		echo "<FONT COLOR=\"red\"><B>ERROR: $msg</B><BR/></FONT>";
	}
	function confirm($msg)
	{
		echo "<FONT COLOR=\"blue\"><B>OK: $msg</B><BR/></FONT>";
	}
	if ( $_POST['action'] == 'Save' )
	{	// white-list of allowed parameters and mapping to GridLAB-D model settings
		$map = array( // this list should match the list of settings needed by the model
			"gridlabd_server" => "HOSTNAME",
			"gridlabd_docroot" => "DOCROOT",
			"gridlabd_port" => "PORT", 
			"glm_modelname" => "NAME",
			"glm_weather" => "WEATHER", 
			"glm_timezone" => "TIMEZONE",
			"glm_loads" => "LOADS", 
			"glm_solar" => "SOLAR", 
			"glm_storage" => "STORAGE", 
			"glm_demand" => "DR_ENABLE",
			"glm_hvac" => "DR_HVAC", 
			"glm_waterheater" => "DR_WATERHEATER", 
			"glm_dishwasher" => "DR_DISHWASHER", 
			"glm_washer" => "DR_WASHER", 
			"glm_dryer" => "DR_DRYER",
			"gas_heating" => "GAS_HEATING",
			"gas_waterheater" => "GAS_WATERHEATER",
			"gas_dryer" => "GAS_DRYER",
			"mysql" => "MYSQL_ENABLE", 
			"my_name" => "MYSQL_NAME", 
			"my_scada" => "MYSQL_SCADA", 
			"my_ami" => "MYSQL_AMI", 
			"my_model" => "MYSQL_MODEL",
			"my_graph" => "MYSQL_GRAPH",
			);
		// update config.php
		$fh = fopen("config/local.php","w");
		$now = date(DATE_RFC2822);
		fwrite($fh,"<?php // do not edit -- updated automatically by 'save' action to setup.php at $now \n");
		foreach ( $_POST as $key => $value )
		{	
			if ( array_key_exists($key,$map) )
			{
				fwrite($fh,"\t$$key='$value';\n");
			}
		}
		fwrite($fh,"?>\n");
		fclose($fh);
		// update config.glm
		$fh = fopen("config/local.glm","w");
		$now = date(DATE_RFC2822);
		fwrite($fh,"// do not edit -- updated automatically by 'save' action to setup.php at $now \n");
		foreach ( $_POST as $key => $value )
		{	// TODO: secure this better by checking the keys against white list of allowed keys
			if ( array_key_exists($key,$map) )
			{
				fwrite($fh,"#define $map[$key]=$value\n");
			}
		}
		fclose($fh);
	}
	else if ( $_POST['action'] == 'Stop' )
	{
		wget("http://$gridlabd_server:$gridlabd_port/control/stop");
		sleep(1);
	}
	else if ( $_POST['action'] == 'Restart' )
	{
		wget("http://$gridlabd_server:$gridlabd_port/control/stop");
		sleep(1);
		passthru("$gridlabd_bin/gridlabd $www_modelpath/$glm_modelname 1>output/gridlabd.log 2>&1 &");
		sleep(1);
	}
	else if ( $_POST['action'] == 'Stop' )
	{
		wget("http://$gridlabd_server:$gridlabd_port/control/stop");
		sleep(1);
	}
	else if ( $_POST['action'] == 'Start' )
	{
		passthru("$gridlabd_bin/gridlabd $www_modelpath/$glm_modelname 1>output/gridlabd.log 2>&1 &");
		sleep(1);
	}
	else if ( $_POST['action'] == 'Cancel' || $_POST['action'] == 'Reset' || $_POST['action'] == 'Refresh' )
	{
		// no action
	}
	else if ( $_POST['action'] != '' )
	{
		error("Action is not valid");
	}
?>
</PRE>
<?php
	if ( $_POST['action'] != 'Reset' && file_exists("config/local.php") ) include("config/local.php");
	function wget($url)
	{
		$ch = curl_init($url);
        curl_setopt($ch, CURLOPT_RETURNTRANSFER, true) ;
		curl_setopt($ch,CURLOPT_TIMEOUT,1);
		curl_setopt($ch, CURLOPT_HEADER, 0);
		$result = curl_exec($ch);
		$info = curl_getinfo($ch);
		if ( $result === false || $info['http_code'] != 200 )
		{
			$code = $info['http_code'];
			$error = curl_error($ch);
			$result = "ERROR $code: $error";
		}
		curl_close($ch);
		return $result;
	}
?>
<HEAD>
<STYLE>
	TH,CAPTION {
		color: black;
		text-align: left;
		font-weight: bold;
		vertical-align: top;
	}
	TD {
		color: blue;
		text-align: left;
	}
	INPUT,SELECT,OPTION {
		color: black;
	}
</STYLE>
<TITLE>GridLAB-D&trade; Realtime Simulation</TITLE>
</HEAD>
<SCRIPT type="text/javascript">
	function refresh(timeout)
	{
	    if ( timeout > 0 ) setTimeout("refresh()",timeout);
	    // TODO
	}
	refresh(1000);
</SCRIPT>
<BODY>
<H1>
[Home]
[<A HREF="<?php echo "http://$gridlabd_server:$gridlabd_port/rt/control.htm";?>">Control</A>]
[<A HREF="<?php echo "http://$gridlabd_server:$gridlabd_port/rt/weather.htm";?>">Weather</A>]
[<A HREF="<?php echo "http://$gridlabd_server:$gridlabd_port/rt/feeder.htm";?>">Feeder</A>]
[<A HREF="<?php echo "http://$gridlabd_server:$gridlabd_port/rt/meter.htm";?>" >Meter</A>]
[<A HREF="<?php echo "http://$gridlabd_server:$gridlabd_port/kml/output/gridlabd.kml";?>">Map</A>]
</H1>
<FORM METHOD="POST">
<INPUT TYPE="hidden" NAME="gridlabd_docroot" VALUE="<?php echo str_replace('/index.php','',$_SERVER['REQUEST_URI']);?>"/>
<INPUT TYPE="hidden" NAME="gridlabd_server" VALUE="<?php echo $gridlabd_server;?>"/>
<INPUT TYPE="hidden" NAME="glm_modelname" VALUE="<?php echo $glm_modelname;?>"/>
<TABLE CELLSPACING=5>
<TR><TD COLSPAN=3><HR/></TD</TR>
<TR><TH COLSPAN=3><H2>Server Setup</H2></TH></TR>
<TR><TH>Hostname</TH><TD>http://<INPUT TYPE="text" NAME="gridlabd_server" VALUE="<?php echo $gridlabd_server;?>" />/</TD><TD>The server hostname to use for GridLAB-D data queries.</TD></TR>
<TR><TH>Port</TH><TD><INPUT TYPE="text" NAME="gridlabd_port" VALUE="<?php echo $gridlabd_port;?>" /></TD><TD>The server TCP port to use for GridLAB-D data queries.</TD></TR>
<TR><TH>Version</TH><TD COLSPAN=2><?php system("$gridlabd_bin/gridlabd --version || echo '<FONT COLOR=RED>(gridlabd error)</FONT>'");?></TD></TR>
<TR><TH></TH><TD COLSPAN=2>Remote: <A HREF="<?php system("git remote get-url $(git remote | head -1)");?>" TARGET="_blank"><?php system("git remote get-url $(git remote | head -1)");?></A></TD></TR>
<TR><TH>Job list <INPUT TYPE="submit" NAME="action" VALUE="Refresh" ONCLICK="location.reload(true);" /></TH><TD COLSPAN=2><PRE>PROC PID   PROGRESS   STATE   CLOCK                   MODEL
---- ----- ---------- ------- ----------------------- ---------------------------------------------------
<DIV ID="joblist"><?php system("$gridlabd_bin/gridlabd --plist || echo '<FONT COLOR=RED>Error</FONT>'");?></DIV></PRE></TD></TR>
<TR><TH>Status</TH>
<TD COLSPAN=2><DIV ID="status">
<?php 
	$rtm=wget("http://$gridlabd_server:$gridlabd_port/raw/realtime_metric"); 
	if ( ! file_exists('config/local.php') )
	{
		error("config/local.php is missing -- have you configured your server properly?");
	}
	if ( ! file_exists('config/local.glm') )
	{
		error("config/local.glm is missing -- have you configured your server properly?");
	}
	if ( file_exists('output') )
	{
		if ( $rtm > 0.0 && $rtm <= 1.0) 
		{
			echo 'Realtime server is running<BR/><INPUT TYPE="submit" NAME="action" VALUE="Stop"/> <INPUT TYPE="submit" NAME="action" VALUE="Restart"/>';
		}
		else if ( substr($rtm,0,5)!="ERROR" )
		{
			echo 'Realtime server startup in progress<BR/><INPUT TYPE="submit" NAME="action" VALUE="Refresh"/>';
		}
		else
		{
			error("Server is not runningl HTTP error is as follows:<BR/>$rtm"); 
			echo '<BR/><INPUT TYPE="submit" NAME="action" VALUE="Start"/>';
		}
		if ( filesize('output/gridlabd.log')>0 )
			echo ' [<A HREF="/output/gridlabd.log" TARGET=_blank>View Log</A>]';
		else
			echo ' (nothing logged yet)';
	}
	else
	{
		error("output folder is missing -- have you configured your server properly?");
	}
?> 
</DIV></TD></TR>
<TR><TD COLSPAN=3><HR/></TD</TR>
<TR><TH COLSPAN=3><H2>Simulation Location</H2></TH></TR>
<TR><TH>Weather</TH><TD>
	<?php
		if ( file_exists("data") )
		{
			$files = scandir("data");
			echo '<SELECT NAME="glm_weather"><OPTION VALUE="none">(select one)</OPTION>';
			foreach ( $files as $pathname ) 
			{
				$path = pathinfo($pathname);
				$name = $path['basename'];
				$ext = $path['extension'];
				if ( $ext == "tmy2" || $ext == "tmy3" )
				{
					print("<OPTION VALUE=\"$name\"");
					if ( $name == $glm_weather )
					{
						print(" SELECTED");
					}
					print(">$name</OPTION>\n");
				}
			}
			echo '</SELECT><BUTTON ONCLICK="window.showModalDialog(\'more.php\',\'\',\'width=650,height=350\');">More</BUTTON>';
		}
		else
		{
			error("data folder is missing -- have you configured your server properly");
		}	
	?>
</TD><TD>The weather file for the simulation model.</TD></TR>
<TR><TH>Timezone</TH><TD>
	<SELECT NAME="glm_timezone">
	<?php
		print("<OPTION>(select one)</OPTION>");
		$fh = fopen("$gridlabd_share/tzinfo.txt","r");
		while ( !feof($fh) )
		{
			$line = fgets($fh,256);
			if ( substr($line,0,4)==" US/" )
			{
				$locale = trim($line);
				print("<OPTION VALUE=\"$locale\"");
				if ( $locale == $glm_timezone ) 
				{
					print(" SELECTED");
				}
				print(">$locale</OPTION>");
			}
		}
		fclose($fh);
	?>
	</SELECT>
</TD><TD>The timezone in which the simulation model runs.</TD></TR>

<TR><TD COLSPAN=3><HR/></TD</TR>
<TR><TH COLSPAN=3><H2>Model setup</H2></TH></TR>
<TR><TH>Model name</TH><TD>
	<SELECT NAME="glm_modelname">
	<?php
		$files = scandir($www_modelpath);
		print('<OPTION VALUE="none">(select one)</OPTION>');
		foreach ( $files as $pathname ) 
		{
			$path = pathinfo($pathname);
			$name = $path['basename'];
			$ext = $path['extension'];
			if ( $ext == "glm" )
			{
				print("<OPTION VALUE=\"$name\"");
				if ( $name == $glm_modelname )
				{
					print(" SELECTED");
				}
				print(">$name</OPTION>\n");
			}
		}
	?>
	</SELECT>
</TD><TD>The simulation model to use when starting the server.</TD></TR>
<TR><TH>Dynamic loads</TH><TD><INPUT TYPE="checkbox" NAME="glm_loads" <?php if($glm_loads){echo" CHECKED";}?>/></TD><TD>Enable dynamic residential loads.</TD></TR>
<TR><TH>Solar</TH><TD><INPUT TYPE="checkbox" NAME="glm_solar" <?php if($glm_solar){echo" CHECKED";}?>/></TD><TD>Enable residential rooftop solar panels.</TD></TR>
<TR><TH>Storage</TH><TD><INPUT TYPE="checkbox" NAME="glm_storage" <?php if($glm_storage){echo" CHECKED";}?>/></TD><TD>Enable residential battery storage.</TD></TR>
<TR><TH>Demand response</TH><TD><INPUT TYPE="checkbox" NAME="glm_demand" <?php if($glm_demand){echo" CHECKED";}?>/></TD><TD>Enable residential demand response.</TD></TR>
<!-- TODO: disable suboptions when demand response is disabled -->
<TR><TH>&nbsp;&nbsp;HVAC response</TH><TD>
	<SELECT NAME="glm_hvac">
		<OPTION>(select one)</OPTION>
		<OPTION VALUE="none" <?php if($glm_hvac=="none") echo "SELECTED";?>>None</OPTION>
		<OPTION VALUE="direct" <?php if($glm_hvac=="direct") echo "SELECTED";?>>Direct</OPTION>
		<OPTION VALUE="price" <?php if($glm_hvac=="price") echo "SELECTED";?>>Price</OPTION>
		<OPTION VALUE="frequency" <?php if($glm_hvac=="frequency") echo "SELECTED";?>>Frequency</OPTION>
	</SELECT>
	<INPUT TYPE="checkbox" NAME="gas_heating" <?php if ($gas_heating){echo"CHECKED";}?> /> Gas heat
</TD><TD>HVAC demand response.</TD></TR>
<TR><TH>&nbsp;&nbsp;Waterheater response</TH><TD>
	<SELECT NAME="glm_waterheater">
		<OPTION>(select one)</OPTION>
		<OPTION VALUE="none" <?php if($glm_waterheater=="none") echo "SELECTED";?>>None</OPTION>
		<OPTION VALUE="direct" <?php if($glm_waterheater=="direct") echo "SELECTED";?>>Direct</OPTION>
		<OPTION VALUE="price" <?php if($glm_waterheater=="price") echo "SELECTED";?>>Price</OPTION>
		<OPTION VALUE="frequency" <?php if($glm_waterheater=="frequency") echo "SELECTED";?>>Frequency</OPTION>
	</SELECT>
	<INPUT TYPE="checkbox" NAME="gas_waterheater" <?php if ($gas_waterheater){echo"CHECKED";}?>/> Gas heat
</TD><TD>Waterheater demand response type.</TD></TR>
<TR><TH>&nbsp;&nbsp;Dishwasher response</TH><TD>
	<SELECT NAME="glm_dishwasher">
		<OPTION>(select one)</OPTION>
		<OPTION VALUE="none" <?php if($glm_dishwasher=="none") echo "SELECTED";?>>None</OPTION>
		<OPTION VALUE="direct" <?php if($glm_dishwasher=="direct") echo "SELECTED"?>>Direct</OPTION>
		<OPTION VALUE="price" <?php if($glm_dishwasher=="price") echo "SELECTED";?>>Price</OPTION>
		<OPTION VALUE="frequency" <?php if($glm_dishwasher=="frequency") echo "SELECTED";?>>Frequency</OPTION>
	</SELECT>
</TD><TD>Dishwasher demand response type.</TD></TR>
<TR><TH>&nbsp;&nbsp;Washer response</TH><TD>
	<SELECT NAME="glm_washer">
		<OPTION>(select one)</OPTION>
		<OPTION VALUE="none" <?php if($glm_washer=="none") echo "SELECTED";?>>None</OPTION>
		<OPTION VALUE="direct" <?php if($glm_washer=="direct") echo "SELECTED";?>>Direct</OPTION>
		<OPTION VALUE="price" <?php if($glm_washer=="price") echo "SELECTED";?>>Price</OPTION>
		<OPTION VALUE="frequency" <?php if($glm_washer=="frequency") echo "SELECTED";?>>Frequency</OPTION>
	</SELECT>
</TD><TD>Washer demand response type.</TD></TR>
<TR><TH>&nbsp;&nbsp;Dryer response</TH><TD>
	<SELECT NAME="glm_dryer">
		<OPTION>(select one)</OPTION>
		<OPTION VALUE="none" <?php if($glm_dryer=="none") echo "SELECTED";?>>None</OPTION>
		<OPTION VALUE="direct" <?php if($glm_dryer=="direct") echo "SELECTED";?>>Direct</OPTION>
		<OPTION VALUE="price" <?php if($glm_dryer=="price") echo "SELECTED";?>>Price</OPTION>
		<OPTION VALUE="frequency" <?php if($glm_dryer=="frequency") echo "SELECTED";?>>Frequency</OPTION>
	</SELECT>
	<INPUT TYPE="checkbox" NAME="gas_dryer" <?php if ($gas_dryer){echo"CHECKED";}?>/> Gas heat
</TD><TD>Dryer demand response type.</TD></TR>

<TR><TD COLSPAN=3><HR/></TD</TR>
<TR><TH COLSPAN=3><H2>MySQL output</H2></TH></TR>
<!-- TODO: disable suboptions when mysql is disabled -->
<TR><TH>MySQL data</TH><TD><INPUT TYPE="checkbox" NAME="mysql" <?php if($mysql)echo"CHECKED";?> /></TD><TD>Enable MySQL data collection.</TD></TR>
<TR><TH>&nbsp;&nbsp;Name</TH><TD><INPUT TYPE="text" NAME="my_name" VALUE="<?php echo $my_name;?>" /></TD><TD>Basename of tables for data collection.</TD></TR>
<TR><TH>&nbsp;&nbsp;SCADA</TH><TD><INPUT TYPE="checkbox" NAME="my_scada" <?php if($my_scada)echo"CHECKED";?> /></TD><TD>Enable SCADA data collection.</TD></TR>
<TR><TH>&nbsp;&nbsp;AMI</TH><TD><INPUT TYPE="checkbox" NAME="my_ami" <?php if($my_ami)echo"CHECKED";?> /></TD><TD>Enable AMI data collection.</TD></TR>
<TR><TH>&nbsp;&nbsp;Model</TH><TD><INPUT TYPE="checkbox" NAME="my_model" <?php if($my_model)echo"CHECKED";?> /></TD><TD>Enable complete model dump.</TD></TR>
<TR><TH>&nbsp;&nbsp;Graph</TH><TD><INPUT TYPE="checkbox" NAME="my_graph" <?php if($my_graph)echo"CHECKED";?> /></TD><TD>Enable complete graph dump.</TD></TR>
<TR><TH>MySQL host</TH><TD><?php echo $my_server;?></TD><TD>Use this host name to read the data from the MySQL server</TD></TR>
<TR><TH>MySQL user</TH><TD><?php echo $my_user;?></TD><TD>Use this user name to read the data from the MySQL server</TD></TR>
<TR><TH>MySQL password</TH><TD><?php echo $my_pwd;?></TD><TD>Use this password to read the data from the MySQL server</TD></TR>

<TR><TD COLSPAN=3><HR/></TD</TR>
<TR><TH>Actions</TH><TD>
	<INPUT TYPE="submit" NAME="action" VALUE="Save" />
	<INPUT TYPE="submit" NAME="action" VALUE="Cancel" />
	<INPUT TYPE="submit" NAME="action" VALUE="Reset" />
</TD></TR>
</TABLE>
<TABLE WIDTH="100%">
<TR><TD COLSPAN=3><HR/></TD</TR>
<TR><TD WIDTH="33%" ALIGN=LEFT><?php system("httpd -V | head -1");?></TD>
	<TD WIDTH="33%" ALIGN=CENTER>Platform: <?php system("uname -smpr");?></TD>
	<TD WIDTH="33%" ALIGN=RIGHT>Date/Time: <?php system("date");?></TD></TR>
<TR><TD COLSPAN=3><HR/></TD</TR>
</TABLE>
</FORM> 

</BODY>
</HTML>
