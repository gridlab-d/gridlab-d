#!/bin/gawk -f

# This file extracts all TROUBLESHOOT comments from input files 
# and outputs the sorted results in html format

BEGIN { # this is executed before any files are processed
	filename = "";
	group = "";
	debug = 1; # 0=disable inline comments, 1=enable inline comments
	print "<link href=\"troubleshooting.css\" rel=\"stylesheet\" type=\"text/css\">"
}

# the following statements, until END, are executed for each line of each file

{ group = ""; } # reset the group for the next line

# check if the current line contains a message, if so, set group to the message type
/.*(gl|output)_warning\(".+".*\)/ { # if the message is gl_warning or output_warning
	group = "Warnings";
}
/.*(gl|output)_error\(".+".*\)/ { # if the message is gl_error or output_error
	group = "Errors";
}
/.*(gl|output)_fatal\(".+".*\)/ { # if the message is gl_fatal or output_fatal
	group = "Fatal errors";
}
/.*(gl_throw|throw_exception)\(".+".*\)/ { # if the message is gl_throw or throw_exception
	group = "Exceptions";
}
/.*throw ".+"/ { # if the message is throw
	group = "Exceptions";
}

{
	# get the module and filename - it is assumed that this script will be 
	# run from the trunk (or branch) and that arguments will be in the format
	# "module/filename" - see generate_troubleshooting.sh
	if ( module "/" filename != FILENAME ) { # if at the beginning of a new file
		split(FILENAME,part,"/");
		module = part[1];
		filename = part[2];
		if ( debug ) print "<!-- OPEN " module "/" filename " -->";
	}

	# process the current line if a message was found
	if ( group != "" ) {

		tag = "l" FNR;
		id = module "/" filename "(" FNR ")";
		if ( debug ) print "<!-- " id ": " gensub(/[ \t]/," ","g",$0);

		# extract the message
		split($0,part,"\"");
		message = part[2];
		if ( debug ) print "\tgroup   = " group;
		if ( debug ) print "\tmessage = " message;
		gsub(/\\"/,"'",message); # replace all double quotes with single quotes in the message
		gsub(/%[0-9.]*s/,"<i>string</i>",message); # replace %s with "string"
		gsub(/%[0-9.]*d/,"<i>integer</i>",message); # replace %d with "integer"
		gsub(/%[0-9.]*i/,"<i>integer</i>",message); # replace %i with "integer"
		gsub(/%[0-9.]*g/,"<i>real</i>",message); # replace %g with "real"
		gsub(/%[0-9.]*f/,"<i>real</i>",message); # replace %f with "real"
		gsub(/%[0-9.]*c/,"<i>character</i>",message); # replace %c with "character"
		message = gensub(/<[^><]*>[^><]*<[^><]*>/,"'&'","g",message); # put single quotes around any text with html tags 
		gsub(/'+/,"'",message); # remove any duplicate single quotes

		# check for TROUBLESHOOT tag
		getline; # get the next line
		if ( match($0, /\/\*[ \t]+TROUBLESHOOT/) == 0 ) {
			if ( debug ) print "No TROUBLESHOOT tag found";
		}
		else { # TROUBLESHOOT tag was found
			explanation = "";
			if ( debug ) print "\texplanation:";
			do {
				text = gensub(/(\/\*[ \t]+TROUBLESHOOT|\*\/)/,"","g",$0); # extract TROUBLESHOOT text
				gsub(/[ \t]+/," ",text); # remove extraneous white space
				gsub(/\\/,"",text); # remove "\\"
				if ( debug ) if ( gensub(/[ \t]/,"","g",text) != "") print "\t" text;
				explanation = explanation text "\n";
				getline;
			}
			while ( index($0,"*/") == 0 )
			
			info = explanation "<cite>See <a href=\"http://gridlab-d.svn.sourceforge.net/viewvc/gridlab-d/trunk/" module "/" filename "?view=markup#" tag "\">" id "</a>.</cite>"
			
			# add message and TROUBLESHOOT text to appropriate array
			if ( group == "Warnings" ) {
				if ( message in warnings )
					print "Repeated message: " message;
				warnings[message] = warnings[message] "\n<LI>" info "</LI>";
				nwarnings++;
			}
			else if ( group == "Errors" ) {
				if ( message in errors )
					print "Repeated message: " message;
				errors[message] = errors[message] "\n<LI>" info "</LI>";
				nerrors++;
			}
			else if ( group == "Fatal errors" ) {
				if ( message in fatal_errors )
					print "Repeated message: " message;
				fatal_errors[message] = fatal_errors[message] "\n<LI>" info "</LI>"; 
				nfatal++;
			}
			else if ( group == "Exceptions" ) {
				if ( message in exceptions )
					print "Repeated message: " message;
				exceptions[message] = exceptions[message] "\n<LI>" info "</LI>";
				nexceptions++;
			}
		}
		if ( debug ) print " -->";
	}
}

END { # this is executed after all files have been processed
	if ( debug ) print "<!-- Total number of messages with TROUBLESHOOT comments found: " nwarnings + nerrors + nfatal + nexceptions " -->";
	if ( debug ) print "<!--\tWarnings: " nwarnings " -->";
	if ( debug ) print "<!--\tErrors: " nerrors " -->";
	if ( debug ) print "<!--\tFatal errors: " nfatal " -->";
	if ( debug ) print "<!--\tExceptions: " nexceptions " -->";

	# "tabs" contains links to each section of messages
	tabs = "<HR /><B><A HREF=\"#Warnings\">Warnings</A> | <A HREF=\"#Errors\">Errors</A> | <A HREF=\"#Fatal\">Fatal errors</A> | <A HREF=\"#Exceptions\">Exceptions</A></B><HR />";
	# "table_header" contains format information for a table that will contain links to messages in each section
	table_header = "<TABLE STYLE=\"BACKGROUND-COLOR:YELLOW\" BORDER=\"1\" CELLPADDING=\"5\" CELLSPACING=\"0\"><TR>";

	print "This troubleshooting guide lists all the errors and warning messages from GridLAB-D. Simply search for your message and follow the recommendations given."
	print "<CITE>Last updated " strftime() "</CITE>."

	# Sort the messages alphabetically and for each character, add an anchor to the first message starting with that character if it exists
        # Warnings
	print "<BR /><A ID=\"Warnings\"></A>" tabs; # create an anchor for this section and links to all sections
        print "<H1>Warnings</H1>\n" table_header; # print the table header for this section
        i = 1;
        for ( msg in warnings ) { # copy indices into another array
                ndx[i] = msg;
                i++;
        }
	IGNORECASE = 1; # ignore case for the sorting of messages
        n = asort(ndx) # sort by message
	IGNORECASE = 0; 
        for ( i = 1; i <= n; i++ ) { # create entries for each message, adding anchors as necessary
                tag = toupper(substr(ndx[i],1,1));
                if ( !(tag in links) ) {
                        links[tag] = 1; # set to true so no other anchors are created for this character
                        print "<TD><A HREF=\"#Warnings_" tag "\">" tag "</A></TD>";
                        warning_str = warning_str "\n<A ID=\"Warnings_" tag "\"></A><H2>" ndx[i] "</H2><UL>" warnings[ndx[i]] "</UL>";
                }
                else {
                        warning_str = warning_str "\n<H2>" ndx[i] "</H2><UL>" warnings[ndx[i]] "</UL>";
                }
        }
        print "</TR></TABLE><BR />";
        print warning_str;
	delete ndx;
	delete links;

	# Errors
	print "\n<A ID=\"Errors\"></A>" tabs; # create an anchor for this section and links to all sections
	print "<H1>Errors</H1>\n" table_header; # print the table header for this section
	i = 1;
	for ( msg in errors ) { # copy indices into another array
		ndx[i] = msg;
		i++;
	}
	IGNORECASE = 1;
	n = asort(ndx) # sort by message
	IGNORECASE = 0;
	for ( i = 1; i <= n; i++ ) { # create entries for each message, adding anchors as necessary
		tag = toupper(substr(ndx[i],1,1));
		if ( !(tag in links) ) {
			links[tag] = 1; # set to true so no other anchors are created for this character
			print "<TD><A HREF=\"#Errors_" tag "\">" tag "</A></TD>";
			error_str = error_str "\n<A ID=\"Errors_" tag "\"></A><H2>" ndx[i] "</H2><UL>" errors[ndx[i]] "</UL>";	
		}
		else {
			error_str = error_str "\n<H2>" ndx[i] "</H2><UL>" errors[ndx[i]] "</UL>";
		}
	}
	print "</TR></TABLE><BR />";
	print error_str;
        delete ndx;
        delete links;

        # Fatal errors
        print "\n<A ID=\"Fatal\"></A>" tabs; # create an anchor for this section and links to all sections
        print "<H1>Fatal errors</H1>\n" table_header; # print the table header for this section
        i = 1;
        for ( msg in fatal_errors ) { # copy indices into another array
                ndx[i] = msg;
                i++;
        }
	IGNORECASE = 1;
        n = asort(ndx) # sort by message
	IGNORECASE = 0;
        for ( i = 1; i <= n; i++ ) { # create entries for each message, adding anchors as necessary
                tag = toupper(substr(ndx[i],1,1));
                if ( !(tag in links) ) {
                        links[tag] = 1; # set to true so no other anchors are created for this character
                        print "<TD><A HREF=\"#Fatal_" tag "\">" tag "</A></TD>";
                        fatal_str = fatal_str "\n<A ID=\"Fatal_" tag "\"></A><H2>" ndx[i] "</H2><UL>" fatal_errors[ndx[i]] "</UL>";
                }
                else {
                        fatal_str = fatal_str "\n<H2>" ndx[i] "</H2><UL>" fatal_errors[ndx[i]] "</UL>";
                }
        }
        print "</TR></TABLE><BR />";
        print fatal_str;
        delete ndx;
        delete links;

        # Exceptions
        print "\n<A ID=\"Exceptions\"></A>" tabs; # create an anchor for this section and links to all sections
        print "<H1>Exceptions</H1>\n" table_header; # print the table header for this section
        i = 1;
        for ( msg in exceptions ) { # copy indices into another array
                ndx[i] = msg;
                i++;
        }
	IGNORECASE = 1;
        n = asort(ndx) # sort by message
	IGNORECASE = 0;
        for ( i = 1; i <= n; i++ ) { # create entries for each message, adding anchors as necessary
                tag = toupper(substr(ndx[i],1,1));
                if ( !(tag in links) ) {
                        links[tag] = 1; # set to true so no other anchors are created for this character
                        print "<TD><A HREF=\"#Exception_" tag "\">" tag "</A></TD>";
                        exception_str = exception_str "\n<A ID=\"Exception_" tag "\"></A><H2>" ndx[i] "</H2><UL>" exceptions[ndx[i]] "</UL>";
                }
                else {
                        exception_str = exception_str "\n<H2>" ndx[i] "</H2><UL>" exceptions[ndx[i]] "</UL>";
                }
        }
        print "</TR></TABLE><BR />";
        print exception_str;
        delete ndx;
        delete links;

	print "<HR />";
}

# Notes:
#    1. BUG FIX: for each character, the first message starting with that character needs to have an anchor to it,
#                along with the message and the TROUBLESHOOT text, and all subsequent messages need to have just
#                the message and the TROUBLESHOOT text. However, for subsequent messages, nothing was being printed.
#                An else clause was added that adds the message and TROUBLESHOOT text for subsequent messages.
#    2. the variable "line" was removed because the built in variable FNR does the same thing
#    3. the variable "status" was removed because it was not being used
#    4. several gensub calls were replaced with gsub calls because gsub intrinsically updates the string it is passed
#    5. the "other" section was removed because it is impossible to get a message of that type
#    6. by using the built in variable IGNORECASE, a lowercase copy of each message was no longer needed (since the 
#       asort function is case sensitive unless IGNORCASE is set to 1)
#    7. PERSISTING ISSUES:
#          - some format specifiers (i.e. %lld in powerflow/frequency_gen.cpp(425)) are not caught
#          - in messages with multiple strings (i.e. core/timestamp.c(693)), only the first string is caught - it is
#            impossible to catch this because any other strings on the line may or may not be part of the message
