#!/bin/gawk -f
BEGIN {
        status = 0; # 0=inactive; 1=active
        filename = "";
        debug = 1; # enable inline comments
        group = "";
        print "<link href=\"troubleshooting.css\" rel=\"stylesheet\" type=\"text/css\">"
}

/.*(gl|output)_fatal\(".+".*\)/ {
        group = "Fatal errors";
}

/.*(gl|output)_error\(".+".*\)/ {
        group = "Errors";
}

/.*(gl|output)_warning\(".+".*\)/ {
        group = "Warnings";
}

/.*(gl_throw|throw_exception)\(".+".*\)/ {
        group = "Exceptions";
}

/.*throw ".+"/ {
        group = "Exceptions";
}

{
        if ( module "/" filename != FILENAME ) {
                split(FILENAME,part,"/");
                module = part[1];
                filename = part[2];
                line = 1;
                status = 0;
                if (debug) print "<!-- OPEN " module "/" filename "-->"
        }
        else {
                line++;
        }
        if ( group != "" ) {
                id = module "/" filename "(" line ")"
                tag = "l_" line;
                if (debug) print "<!-- " id ": " gensub(/[ \t]+/," ","g",$0);
                split($0,part,"\"");
                message = part[2];
                if (debug) print "     group   = " group;
                if (debug) print "     message = " message;
				message = gensub(/\\"/,"'","g",message);
                getline; line++;
                if ( match($0,/\/\*[ \t]+TROUBLESHOOT/)==0) {
                        if (debug) print "    no TROUBLESHOOT tag found";
                }
                else {
                        if (debug) print "     explanation:"
                        explanation = "";
                        do {
                                text = gensub(/(\/\*[ \t]+TROUBLESHOOT|\*\/)/,"","g",$0);
                                text = gensub(/[ \t]+/," ", "g", text);
								text = gensub(/\\/,"","g",text);
                                if (debug) print "    " text;
                                explanation = explanation text "\n";
                                getline; line++;
                        }
                        while (index($0,"*/")==0)
						info = explanation "<cite>See <a href=\"http://gridlab-d.svn.sourceforge.net/viewvc/gridlab-d/trunk/" module "/" filename "?view=markup#" tag "\">" id "</a>.</cite>"
                        #output[group] = output[group] "<DT>" message "</DT>" item;
						if (group=="Fatal errors")
							fatal_errors[message] = fatal_errors[message] "\n<LI>" info "</LI>";
						else if (group=="Errors")
							errors[message] = errors[message] "\n<LI>" info "</LI>";
						else if (group=="Warnings")
							warnings[message] = warnings[message] "\n<LI>" info "</LI>";
						else if (group=="Exceptions")
							exceptions[message] = exceptions[message] "\n<LI>" info "</LI>";
						else
							other[message] = other[message] "\n<LI>" item "</LI>";
                }
                if (debug) print "  -->"
                group = "";
        }
}


END {
	alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	tabs = "<HR/><B><A HREF=\"#Warnings\">Warnings</A> | <A HREF=\"#Errors\">Errors</A> | <A HREF=\"#Fatal\">Fatal errors</A> | <A HREF=\"#Exceptions\">Exceptions</A> | <A HREF=\"#Other\">Other messages</A></B><HR/>";
	links = "";
	for (i=1; i<=26; i++) links = links "<A HREF=\"#Warnings_" substr(alphabet,i,1) "\">" substr(alphabet,i,1)  "</A> | ";
        print "This troubleshooting guide lists all the errors and warning messages from GridLAB-D.  Simply search for your message and follow the recommendations given."
	print "<CITE>Last updated " strftime() "</CITE>."
	print "<BR><A ID=\"Warnings\"></A>" tabs;
	print "<H1>Warnings</H1>" links;
	print "<BR/>"
	i=1
	for (msg in warnings) {
		m = tolower(msg);
		txt[m] = warnings[msg];
		ndx[i++] = m;
		hdg[m] = msg;
	}
	n = asort(ndx);
	for (i=1; i<=n; i++) print "\n<A ID=\"Warnings_" substr(hdg[ndx[i]],1,1) "\"></A><H2>" hdg[ndx[i]] "</H2><UL>" txt[ndx[i]] "</UL>";

	print "<A ID=\"Errors\"></A>" tabs;
	print "<H1>Errors</H1>" links;
	i=1
	for (msg in errors) {
		m = tolower(msg);
		txt[m] = errors[msg];
		ndx[i++] = m;
		hdg[m] = msg;
	}
	n = asort(ndx);
	for (i=1; i<=n; i++) print "\n<A ID=\"Errors_" substr(hdg[ndx[i]],1,1) "\"></A><H2>" hdg[ndx[i]] "</H2><UL>" txt[ndx[i]] "</UL>";

	print "<A ID=\"Fatal\"></A>" tabs;
	print "<H1>Fatal errors</H1>" links;
	i=1
	for (msg in fatal_errors) {
		m = tolower(msg);
		txt[m] = fatal_errors[msg];
		ndx[i++] = m;
		hdg[m] = msg;
	}
	n = asort(ndx);
	for (i=1; i<=n; i++) print "\n<A ID=\"Fatal_" substr(hdg[ndx[i]],1,1) "\"></A><H2>" hdg[ndx[i]] "</H2><UL>" txt[ndx[i]] "</UL>";

	print "<A ID=\"Exceptions\"></A>" tabs;
	print "<H1>Exceptions</H1>" links;
	i=1
	for (msg in exceptions) {
		m = tolower(msg);
		txt[m] = exceptions[msg];
		ndx[i++] = m;
		hdg[m] = msg;
	}
	n = asort(ndx);
	for (i=1; i<=n; i++) print "\n<A ID=\"Exceptions_" substr(hdg[ndx[i]],1,1) "\"></A><H2>" hdg[ndx[i]] "</H2><UL>" txt[ndx[i]] "</UL>";

	print "<A ID=\"Other\"></A>" tabs;
	print "<H1>Other messages</H1>" links;
	i=1
	for (msg in other) {
		m = tolower(msg);
		txt[m] = others[msg];
		ndx[i++] = m;
		hdg[m] = msg;
	}
	n = asort(ndx);
	for (i=1; i<=n; i++) print "\n<A ID=\"Other_" substr(hdg[ndx[i]],1,1) "\"></A><H2>" hdg[ndx[i]] "</H2><UL>" txt[ndx[i]] "</UL>";
	print "<HR/>";
}

