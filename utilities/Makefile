# $Id: Makefile,v 1.4 2008/02/12 18:56:56 d3g637 Exp $
# Module creation utilities

install:
	cp add_module add_class add_module.d/*.* add_class.d/*.* /usr/local/bin

# troubleshooting guide generation
troubleshooting:
	( cd .. ; mkdir -p ../documents ; awk -f ./utilities/troubleshooting.awk */*.{c,cpp} > ../documents/troubleshooting.html )
	cp troubleshooting.css ../../documents/troubleshooting.css

