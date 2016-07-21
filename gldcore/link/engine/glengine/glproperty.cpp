/*
    Copyright (c) 2013, <copyright holder> <email>
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:
        * Redistributions of source code must retain the above copyright
        notice, this list of conditions and the following disclaimer.
        * Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution.
        * Neither the name of the <organization> nor the
        names of its contributors may be used to endorse or promote products
        derived from this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY <copyright holder> <email> ''AS IS'' AND ANY
    EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL <copyright holder> <email> BE LIABLE FOR ANY
    DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


#include "glproperty.h"

#include <sstream>
#include <string.h>

glproperty::glproperty(PROPOERTYCONTEXT context, PROPERTYTYPE type,int index, string name, size_t size, string value)
{
  this->context=context;
  this->type=type;
  this->name=name;
  this->size=size;
  //TODO string to value conversions
  this->value=value;
  this->index=index;
}

glproperty* glproperty::deserialize(char* given)
{
   string context,name,objname,value;
   size_t size;
   int type,index;
   PROPOERTYCONTEXT cont;
   string input(given);
   
   istringstream gstream(input);
   
   gstream >> context >> index >> type >> size >> objname >> name >> value;
   if(gstream.fail()){
     throw "Property parse error!";
   }
   
   if(context.compare("GLOBAL")==0){
      cont=GLOBAL;
    }
    else
    if(context.compare("EXPORT")==0){
    
      cont=EXPORT;
    }
    else
    if(context.compare("IMPORT")==0){
    
      cont=IMPORT;
    }
    else{
    
      throw "Context parse error";
    }
    
    if(type>PT_random){
      throw "Type parse error!";
    }
    
	string fname;
	if(objname=="NULL")
		fname=string(name);
	else
		fname=string(objname+"."+name);

    return new glproperty(cont,(PROPERTYTYPE)type,index,fname,size,string(value));
}

char* glproperty::serialize(glproperty* given)
{
  
  stringstream temp;
  
  if(given->context==GLOBAL)
    temp << "GLOBAL";
  if(given->context==IMPORT)
    temp << "IMPORT";
  if(given->context==EXPORT)
    temp << "EXPORT";
  
  temp << " " << given->index << " " <<given->type << " " << given->name << " " << given->value << "\n";
  
  string temp2=temp.str();
  
  char *toReturn=new char[temp2.length()+1];
  toReturn[temp2.length()]='\0';
  memcpy(toReturn,temp2.c_str(),temp2.length());
  
  return toReturn;
  
}

string glproperty::getName()
{
	return this->name;
}

PROPERTYTYPE glproperty::getType()
{
  return this->type;
}

string glproperty::getValue()
{
  return this->value;
}




