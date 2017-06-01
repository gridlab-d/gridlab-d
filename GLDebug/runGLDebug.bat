rem echo off
set GLCLASSPATH=GLDebug.jar
set GLCLASSPATH=%GLCLASSPATH%;.\lib\jdom.jar

java -Xmx200m -classpath %GLCLASSPATH% gov.pnnl.gridlab.gldebug.GLDebug
