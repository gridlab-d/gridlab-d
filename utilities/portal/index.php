<HTML>

<HEAD>
<TITLE>GridLAB-D Job Portal</TITLE>
</HEAD>

<BODY>

<?php
	function wfile($fname,$str)
	{
		$fh = fopen($fname,"w");
		fwrite($fh,$str);
		fclose($fh);
	}

	$config = file('config');
	foreach ( $config as $line )
	{
		eval("$".$line.";");
	}

	// header
	echo "<TABLE WIDTH=\"100%\"><TR>";

	// get user info
	$USER = $_SERVER['PHP_AUTH_USER'];
	echo "<TD>Logged in as: <B>$USER</B></TD>";

	echo "<TH>GridLAB-D&trade; job control</TH>"; 

	// get version info
	$version = exec("echo $(gridlabd --version | cut -f2 -d:)");
	echo "<TD ALIGN=\"right\">$version</TD>";
	echo "</TR></TABLE><HR/>";

	// check for file upload
	if ( array_key_exists('jobfile',$_FILES) )
	{
		$jobfile = $_FILES['jobfile'];
		switch ( $jobfile['error'] ) {
		case 0:	
			// need to lock this before getting the job no.
			if ( file_exists('jobs/.nextjob') )
				$jobno = trim(file_get_contents('jobs/.nextjob'));
			else
				$jobno = 1;
			$jobdir = "jobs/$jobno";
			if ( $jobfile['type'] != "application/zip" )
				$error = "Uploaded files must be in ZIP format";
			elseif ( !mkdir($jobdir) )
				$error = "Unable to create folder for job no. $jobno";
			elseif ( !chmod($jobdir,0770) )
				$error = "Unable to set job permissions for job no. $jobno.";
			elseif ( !move_uploaded_file($jobfile['tmp_name'],"$jobdir/jobfile.zip") )
			{
				$error = "Unable to move jobfile to $jobdir";
				wfile("$jobdir/.status","Error");
			}
			else
			{			
				// setup job
				wfile("$jobdir/.status","Pending");
				wfile("$jobdir/.description","Name: ".$jobfile['name']."; ");
				wfile("$jobdir/.htaccess","Require user $USER");
				wfile("jobs/.nextjob",$jobno+1);
				$message = "File <B>" . $jobfile['name'] . "</B> uploaded ok. Job <A HREF=\"#$jobno\">no. $jobno</A> queued.";
			}
			break;
		case 1:
			$error = "File exceeds server limit";
			break;
		case 2:
			$error = "File exceeds client limit";
			break;
		case 3:
			$error = "File only partially uploaded";
			break;
		case 4:
			$error = "No file uploaded";
			break;
		case 5:
			$error = "Missing temporary folder";
			break;
		case 6:
			$error = "Unable to save file";
			break;
		case 7:
			$error = "Upload stopped by server";
			break;
		default:
			$error = "Unknown error code (code=" . $jobfile['error'] . ")";
			break;
		}
	}

	if ( array_key_exists('action',$_GET) )
	{
		$action = $_GET['action'];
		switch ( $action ) {
		case 'Del':
			if ( array_key_exists('job',$_GET) )
			{
				$job = $_GET['job'];
				$ans = exec(escapeshellcmd("rm -rf 'jobs/$job'"));
				if ( $ans == "" )
					$message = "Job $job deleted.";
				else	
					$error = "Unable to delete job $job";
			}
			else
				$error = "Missing job number to delete";
		case 'Start':
			if ( $USER == "admin" )
				exec("( ./gldq start & ) 1>>.log 2>&1 &");
			else
				$error = "Permission denied";
			sleep(1);
			break;
		case 'Stop':
			if ( $USER == "admin" )
				exec("(./gldq stop & ) 1>>.log 2>&1 &");
			else
				$error = "Permission denied";
			sleep(1);
			break;
		case 'Clear':
			if ( $USER == "admin" )
				exec("echo \"$(date): log cleared\" >.log");
			else
				$error = "Permission denied";
			break;
		case 'Reset':
			if ( $USER == "admin" )
				exec("(./gldq reset & ) 1>>.log 2>&1 &");
			else
				$error = "Permission denied";
			break;
		case 'Save':
			if ( $USER == "admin" )
				exec("(./gldq save & ) 1>>.log 2>&1 &");
			else
				$error = "Permission denied";
			$message="Accounting data saved";
			break;
		default:
			$error = "Invalid action";
			break;
		}
	}

	echo "<TABLE WIDTH=\"100%\"><TR><TH ALIGN=\"left\">";
	if ( isset($error) )
		echo "<FONT COLOR=RED>$error</FONT>";
	elseif ( isset($message) )
		echo "<FONT COLOR=BLUE>$message</FONT>";
	else
		echo "<FONT COLOR=BLUE>Ready.</FONT>";
	echo "</TH><TD ALIGN=\"right\">System load: <B>";
	$load = exec("uptime | cut -f5 -d,");
	if ( isset($NUMPROCS) )
		printf("%.0f%%",$load/$NUMPROCS*100);
	else
		echo $load;
	echo "</B></TD></TR>";
?>

<TR><TD>
<FORM METHOD="get" ACTION="/gridlabd">
<INPUT TYPE="submit" VALUE="Refresh"/>
Queue is
<?php
	if ( file_exists('.status') )
	{
		readfile('.status');
		if ( $USER == "admin" )
			echo ' <INPUT TYPE="submit" NAME="action" VALUE="Stop" />';
	}		
	else
	{
		echo "off";
		if ( $USER == "admin" )
			echo ' <INPUT TYPE="submit" NAME="action" VALUE="Start" />';
	}
	if ( $USER == "admin" )
	{
		echo '<INPUT TYPE="submit" NAME="action" VALUE="Reset" />';
		echo '<A HREF=".log" TARGET="new">View log</A>';
		echo '<INPUT TYPE="submit" NAME="action" VALUE="Clear" />';
		echo '<A HREF="/gridlabd/acct" TARGET="new">Accounting</A>';
		echo '<INPUT TYPE="submit" NAME="action" VALUE="Save" />';
	}
?>
</FORM>
</TD><TD ALIGN="right"><A HREF="help.php" TARGET="new">Help</TD></TR>
</TABLE>

<TABLE BORDER="1" WIDTH="100%">
<TR><TH WIDTH="50">Job no.</TH><TH WIDTH="100">User</TH><TH WIDTH="100">Status</TH><TH COLSPAN=2 ALIGN="left">Description</TH></TR>
<TR>
<FORM METHOD="post" ENCTYPE="multipart/form-data" >
	<TD><INPUT TYPE="submit" VALUE="Submit"/></TD>
	<TD COLSPAN=4>
		<INPUT NAME="jobfile" TYPE="file" SIZE="50" VALUE="jobfile.zip"/>
	</TD>
</FORM>
</TR>
<?php
	if ( is_dir('jobs') )
	{
		if ( $jobs = opendir('jobs') )
		{
			while ( ($job=readdir($jobs)) !== false )
			{
				if ( substr($job,0,1) !== "." )
				{
					echo "<TR>";
					echo "<FORM METHOD=\"get\" ACTION=\"/gridlabd\" >";
					echo "<INPUT TYPE=\"hidden\" NAME=\"job\" VALUE=\"$job\" />";
					echo "<TD><A NAME=\"$job\"></A>$job ";
					echo "</TD>";
					$tags = array("htaccess","status","description");
					foreach ( $tags as $tag )
					{
						$file = "jobs/$job/.$tag";
						echo "<TD>";
						
						if ( file_exists($file) )
						{
							switch ( $tag ) {
							case "status":
								if ( $user == $USER )
								{
									echo "<A HREF=\"jobs/$job\" TARGET=\"new\">";
									readfile($file);
									echo "</A>";
								}
								else
									readfile($file);
								break;
							case "htaccess":
								$lines = file($file);
								$user = "(na)";
								foreach ( $lines as $num => $line )
								{
									if ( substr($line,0,13) == "Require user " )
										$user = trim(substr($line,13));
								}
								echo $user;
								break;
							default:
								readfile($file);
								if ( $user == $USER || $USER == "admin" )
									echo "</TD><TD><INPUT TYPE=\"submit\" NAME=\"action\" VALUE=\"Del\" />";
								break;
							}
						}
						else
							echo "(na)";
						echo "</TD>";
					}
					echo "</FORM>";
					echo "</TR>";
				}
			}
		}
		else
			echo "<TR><TD COLSPAN=4>No jobs</TD></TR>";

		closedir($jobs);
	}
	else
		echo "Jobs folder is unavailable";


?>
</TABLE> 

</BODY>
</HEAD>

