#!/bin/gawk -f
BEGIN {
        status = 0; # 0=inactive; 1=active
        filename = "";
        debug = 0; # enable inline comments
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
                tag = "l" line;
                if (debug) print "<!-- " id ": " gensub(/[ \t]+/," ","g",$0);
                split($0,part,"\"");
                message = part[2];
                if (debug) print "     group   = " group;
                if (debug) print "     message = " message;
				message = gensub(/\\"/,"'","g",message);
				message = gensub(/%[0-9.]*s/,"<i>string</i>", "g", message);
				message = gensub(/%[0-9.]*d/,"<i>integer</i>", "g", message);
				message = gensub(/%[0-9.]*g/,"<i>real</i>", "g", message);
				message = gensub(/%[0-9.]*f/,"<i>real</i>", "g", message);
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
	tabs = "<HR/><B><A HREF=\"#Warnings\">Warnings</A> | <A HREF=\"#Errors\">Errors</A> | <A HREF=\"#Fatal\">Fatal errors</A> | <A HREF=\"#Exceptions\">Exceptions</A> | <A HREF=\"#Other\">Other messages</A></B><HR/>";
    print "This troubleshooting guide lists all the errors and warning messages from GridLAB-D.  Simply search for your message and follow the recommendations given."
	print "<CITE>Last updated " strftime() "</CITE>."

	i=1; links="<TABLE BGCOLOR=lightyellow BORDER=1 CELLPADDING=5 CELLSPACING=0><TR>";
	for (msg in warnings) {
		m = tolower(msg);
		warnings_ndx[toupper(substr(m,1,1))] = toupper(substr(m,1,1));
		txt[m] = warnings[msg];
		ndx[i++] = m;
		hdg[m] = msg;
	}
    n = asort(warnings_ndx);
	for (i=1; i<=n; i++) {
		links = links "<TD><A HREF=\"#Warnings_" warnings_ndx[i] "\">" warnings_ndx[i]  "</A></TD>";
		warnings_ndx[i] = 0;
	}
	links = links "</TR></TABLE>";
	print "<BR><A ID=\"Warnings\"></A>" tabs NL "<H1>Warnings</H1>" links "<BR/>" NL;
	n = asort(ndx);
	for (i=1; i<=n; i++) {
		tag = toupper(substr(hdg[ndx[i]],1,1));
		if (warnings_ndx[tag]==0) print "\n<A ID=\"Warnings_" tag "\"></A><H2>" hdg[ndx[i]] "</H2><UL>" txt[ndx[i]] "</UL>";
		warnings_ndx[tag] = warnings_ndx[tag] + 1;
	}

	i=1; links="<TABLE BGCOLOR=lightyellow BORDER=1 CELLPADDING=5 CELLSPACING=0><TR>";
	for (msg in errors) {
		m = tolower(msg);
		errors_ndx[toupper(substr(m,1,1))] = toupper(substr(m,1,1));
		txt[m] = errors[msg];
		ndx[i++] = m;
		hdg[m] = msg;
	}
    n = asort(errors_ndx);
	for (i=1; i<=n; i++) {
		links = links "<TD><A HREF=\"#Errors_" errors_ndx[i] "\">" errors_ndx[i]  "</A></TD>";
		errors_ndx[i] = 0;
	}
	links = links "</TR></TABLE>";
	print "<A ID=\"Errors\"></A>" tabs NL "<H1>Errors</H1>" links "<BR/>" NL;
	n = asort(ndx);
	for (i=1; i<=n; i++) {
		tag = toupper(substr(hdg[ndx[i]],1,1));
		if (errors_ndx[tag]==0) print "\n<A ID=\"Errors_" toupper(substr(hdg[ndx[i]],1,1)) "\"></A><H2>" hdg[ndx[i]] "</H2><UL>" txt[ndx[i]] "</UL>";
		errors_ndx[tag] = errors_ndx[tag] + 1;
	}

	i=1; links="<TABLE BGCOLOR=lightyellow BORDER=1 CELLPADDING=5 CELLSPACING=0><TR>";
	for (msg in fatal_errors) {
		m = tolower(msg);
		fatal_ndx[toupper(substr(m,1,1))] = toupper(substr(m,1,1));
		txt[m] = fatal_errors[msg];
		ndx[i++] = m;
		hdg[m] = msg;
	}
    n = asort(fatal_ndx);
	for (i=1; i<=n; i++) {
		links = links "<TD><A HREF=\"#Fatal_" fatal_ndx[i] "\">" fatal_ndx[i]  "</A></TD>";
		fatal_ndx[i] = 0;
	}
	links = links "</TR></TABLE>";
	print "<A ID=\"Fatal\"></A>" tabs NL "<H1>Fatal errors</H1>" links "<BR/>" NL;
	n = asort(ndx);
	for (i=1; i<=n; i++) {
		tag = toupper(substr(hdg[ndx[i]],1,1));
		if (fatal_ndx[tag]==0) print "\n<A ID=\"Fatal_" toupper(substr(hdg[ndx[i]],1,1)) "\"></A><H2>" hdg[ndx[i]] "</H2><UL>" txt[ndx[i]] "</UL>";
		fatal_ndx[tag] = fatal_ndx[tag] + 1;
	}

	i=1; links="<TABLE BGCOLOR=lightyellow BORDER=1 CELLPADDING=5 CELLSPACING=0><TR>";
	for (msg in exceptions) {
		m = tolower(msg);
		exception_ndx[toupper(substr(m,1,1))] = toupper(substr(m,1,1));
		txt[m] = exceptions[msg];
		ndx[i++] = m;
		hdg[m] = msg;
	}
    n = asort(exception_ndx);
	for (i=1; i<=n; i++) {
		links = links "<TD><A HREF=\"#Exceptions_" exception_ndx[i] "\">" exception_ndx[i]  "</A></TD>";
		exception_ndx[i] = 0;
	}
	links = links "</TR></TABLE>";
	print "<A ID=\"Exceptions\"></A>" tabs NL "<H1>Exceptions</H1>" links "<BR/>" NL;
	n = asort(ndx);
	for (i=1; i<=n; i++) {
		tag = toupper(substr(hdg[ndx[i]],1,1));
		if (exception_ndx[tag]==0) print "\n<A ID=\"Exceptions_" toupper(substr(hdg[ndx[i]],1,1)) "\"></A><H2>" hdg[ndx[i]] "</H2><UL>" txt[ndx[i]] "</UL>";
		exception_ndx[tag] = exception_ndx[tag] + 1;
	}

	i=1; links="<TABLE BGCOLOR=lightyellow BORDER=1 CELLPADDING=5 CELLSPACING=0><TR>";
	for (msg in other) {
		m = tolower(msg);
		other_ndx[toupper(substr(m,1,1))] = toupper(substr(m,1,1));
		txt[m] = others[msg];
		ndx[i++] = m;
		hdg[m] = msg;
	}
    n = asort(other_ndx);
	for (i=1; i<=n; i++) {
		links = links "<TD><A HREF=\"#Other_" other_ndx[i] "\">" other_ndx[i]  "</A></TD>";
		other_ndx[i] = 0;
	}
	links = links "</TR></TABLE>";
	print "<A ID=\"Other\"></A>" tabs NL "<H1>Other messages</H1>" links "<BR/>" NL;
	n = asort(ndx);
	for (i=1; i<=n; i++) {
		tag = toupper(substr(hdg[ndx[i]],1,1));
		if (other_ndx[tag]==0) print "\n<A ID=\"Other_" toupper(substr(hdg[ndx[i]],1,1)) "\"></A><H2>" hdg[ndx[i]] "</H2><UL>" txt[ndx[i]] "</UL>";
		other_ndx[tag] = other_ndx[tag] + 1;
	}

	print "<HR/>";
}

