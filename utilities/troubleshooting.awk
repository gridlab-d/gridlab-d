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
                tag = "l_" line;
                if (debug) print "<!-- " id ": " gensub(/[ \t]+/," ","g",$0);
                split($0,part,"\"");
                message = part[2];
                if (debug) print "     group   = " group;
                if (debug) print "     message = " message;
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
                                if (debug) print "    " text;
                                explanation = explanation text "\n";
                                getline; line++;
                        }
                        while (index($0,"*/")==0)
                        output[group] = output[group] "<DT>" message "</DT><DD>" explanation "<cite>See <a href=\"http://gridlab-d.svn.sourceforge.net/viewvc/gridlab-d/trunk/" module "/" filename "?view=markup#" tag "\">" id "</a></cite></DD>";
                }
                if (debug) print "  -->"
                group = "";
        }
}


END {
        print "This troubleshooting guide lists all the errors and warning messages from GridLAB-D.  Simply search for your message and follow the recommendations given."
	print "<CITE>Last updated " strftime() "</CITE>."
        for (group in output) {
                print "<H1>" group "</H1>\n<DL>" output[group] "</DL>";
        }
}

