 

//Xerces initialization file

#include <xercesc/util/PlatformUtils.hpp>
// Other include files, declarations, and non-Xerces-C++ initializations.
XERCES_CPP_NAMESPACE_USE 
  
int main(int argc, char* argv[])
{
  try {
    XMLPlatformUtils::Initialize();
  }
  catch (const XMLException& toCatch) {
    // Do your failure processing here
    return 1;
  }

  // Do your actual work with Xerces-C++ here.

  XMLPlatformUtils::Terminate();

  // Other terminations and cleanup.
  return 0;
}
 