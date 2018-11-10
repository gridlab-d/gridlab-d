<HTML>
<?php
	$map = array(
		"AL" => "Alabama",
		"AK" => "Alaska",
		"AZ" => "Arizona",
		"AR" => "Arkansas",
		"CA" => "California",
		"CO" => "Colorado",
		"CT" => "Connecticut",
		"DE" => "Delaware",
		"FL" => "Florida",
		"GA" => "Georgia",
		"HI" => "Hawaii",
		"ID" => "Idaho",
		"IL" => "Illinois",
		"IN" => "Indiana",
		"IA" => "Iowa",
		"KS" => "Kansas",
		"KY" => "Kentucky",
		"LA" => "Louisiana",
		"ME" => "Maine",
		"MD" => "Maryland",
		"MA" => "Massachusetts",
		"MI" => "Michigan",
		"MN" => "Minnesota",
		"MS" => "Mississippi",
		"MO" => "Missouri",
		"MT" => "Montana",
		"NE" => "Nebraska",
		"NV" => "Nevada",
		"NH" => "New Hampshire",
		"NJ" => "New Jersey",
		"NM" => "New Mexico",
		"NY" => "New York",
		"NC" => "North Carolina",
		"ND" => "North Dakota",
		"OH" => "Ohio",
		"OK" => "Oklahoma",
		"OR" => "Oregon",
		"PA" => "Pennsylvania",
		"RI" => "Rhode Island",
		"SC" => "South Carolina",
		"SD" => "South Dakota",
		"TN" => "Tennessee",
		"TX" => "Texas",
		"UT" => "Utah",
		"VT" => "Vermont",
		"VA" => "Virginia",
		"WA" => "Washington",
		"WV" => "West Virginia",
		"WI" => "Wisconsin",
		"WY" => "Wyoming"
	);
?>
<BODY>
Select states for which climate data is desired:
<FORM METHOD="POST">
<TABLE WIDTH="100%"><TR>
<?php
	$n = 0;
	foreach ( $map as $abbr => $name )
	{
		if ( fmod($n,10)==0 )
			echo "<TD WIDTH=20%>";
		echo "<INPUT TYPE=\"checkbox\" NAME=\"$abbr\" ";
		if ( (array_key_exists("action",$_POST) && array_key_exists($abbr,$_POST)) || (!array_key_exists("action",$_POST) && file_exists("data/$abbr.zip")) )
			echo "CHECKED";
		echo "/>$name<BR/>";
		if ( fmod($n,10)==9 )
			echo "</TD>\n";
		$n += 1;
	}
?>
</TABLE>
<INPUT TYPE="submit" NAME="action" VALUE="Process" />
<BUTTON ONCLICK="window.close();">Done</BUTTON>
<BR/>After you make changes, click [Process] to download weather data and update the weather data folder.  
Click [Done] when you have finished processing new data.
</FORM>
<PRE>
<?php
	if ( array_key_exists("action",$_POST) )
	{
		if ( $_POST["action"] == "Process" )
		{
			foreach ( $map as $abbr => $name )
			{
				if ( array_key_exists($abbr,$_POST) && !file_exists("data/$abbr.zip") )
				{
					echo "Adding $abbr...";
					system("(cd data && curl -s https://raw.githubusercontent.com/gridlab-d/data/master/US/tmy3/$abbr.zip > $abbr.zip && unzip -jn $abbr.zip) 2>&1 1>/dev/null",$rc);
					if ( $rc == 0 )	echo "ok";
					echo "\n";
				}
				else if ( !array_key_exists($abbr,$_POST) && file_exists("data/$abbr.zip") )
				{
					echo "Deleting $abbr...";
					system("(cd data && rm -if $abbr*) 2>&1",$rc);
					if ( $rc == 0 )	echo "ok";					
					echo "\n";
				}
			}
		}
	}
?>
</PRE>
</BODY>
</HTML>