/**	gld_load.cpp
 * Copyright (C) 2008 Battelle Memorial Institute
 * Authors:
 *	Matthew Hauer <matthew.hauer@pnl.gov>, 6 Nov 07 -
 *
 * Versions:
 *	1.0 - MH - initial version
 *
 *	@file load_xml.cpp
 *	@addtogroup xml_load XML file loader
 *	@ingroup core
 *
 **/

#include "platform.h"

#ifdef HAVE_XERCES
#include "load_xml.h"

/* included in here to avoid the dubious joys of C/C++ mixes -mh */
#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/sax2/SAX2XMLReader.hpp>
#include <xercesc/sax2/XMLReaderFactory.hpp>
#include <xercesc/sax2/DefaultHandler.hpp>
#include <xercesc/util/XMLString.hpp>

#include "load_xml_handle.h"

// Other include files, declarations, and non-Xerces-C++ initializations.
XERCES_CPP_NAMESPACE_USE 
  
/*int main(int argc, char** argv){
	char* xmlFile = "well-formatted-new.xml";
	//char *xmlFile = "GridLABD_Multi_Example.xml";
	return loadall_xml(xmlFile);
}*/

STATUS loadall_xml(const char *filename){
	if(filename == NULL)
		return FAILED;
	try{
		XMLPlatformUtils::Initialize();
	} catch(const XMLException& /*toCatch*/){
		output_error("Load_XML: Xerces Initialization failed.");
		output_debug(" * something really spectacularly nasty happened inside Xerces and outside our control.");
		return FAILED;
	}

	SAX2XMLReader* parser = XMLReaderFactory::createXMLReader();
	parser->setFeature(XMLUni::fgXercesDynamic, true);
	//parser->setFeature(XMLUni::fgSAX2CoreValidation, true);   
	//parser->setFeature(XMLUni::fgSAX2CoreNameSpaces, true);   // optional

	auto *defaultHandler = new gld_loadHndl();
	parser->setContentHandler(defaultHandler);
	parser->setErrorHandler(defaultHandler);
	
	try {
		parser->parse(filename);
	} catch (const XMLException& toCatch){
		char* message = XMLString::transcode(toCatch.getMessage());
		output_error("Load_XML: XMLException from Xerces: %s", message);
		XMLString::release(&message);
		return FAILED;
	} catch (const SAXParseException& toCatch){
		char* message = XMLString::transcode(toCatch.getMessage());
		output_error("Load_XML: SAXParseException from Xerces: %s", message);
		XMLString::release(&message);
		return FAILED;
	} catch (...) {
		output_error("Load_XML: unexpected exception from Xerces.");
		return FAILED;
	}
	if(!defaultHandler->did_load()){
		output_error("Load_XML: loading failed.");
		return FAILED;
	}
	delete parser;
	delete defaultHandler;
	return SUCCESS;
}

#endif /* HAVE_XERCES */
/* EOF */
