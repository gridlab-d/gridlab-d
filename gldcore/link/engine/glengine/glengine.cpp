// This is the main DLL file.

#include "glengine.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void glengine::debug(const int level, const char *fmt, ...)
{

	if ( level<=debug_level )
	{
		char output[1024];
		va_list ptr;
		va_start(ptr,fmt);
		vsprintf_s(output,sizeof(output),fmt,ptr);
		va_end(ptr);
		cerr << "DEBUG(" << level << ") [glengine]: " << output << endl;
	}
}

void glengine::warning(const char *fmt, ...)
{
	char output[1024];
	va_list ptr;
	va_start(ptr,fmt);
	vsprintf_s(output,sizeof(output),fmt,ptr);
	va_end(ptr);
	cerr << "WARNING  [glengine]: " <<  output << endl; 
}

//instead of error, I use exceptions. Let the client deal with the error
/*int glengine::error(const char *fmt, ...)
{
	char output[1024];
	va_list ptr;
	va_start(ptr,fmt);
	vsprintf_s(output,sizeof(output),fmt,ptr);
	va_end(ptr);
	return fprintf(stderr,"ERROR    [glengine] %s\n",output);
}*/

void glengine::exception(const char *fmt, ...)
{
	static char output[1024];
	va_list ptr;
	va_start(ptr,fmt);
	vsprintf_s(output,sizeof(output),fmt,ptr);
	va_end(ptr);
	throw (const char*)output;
}


glengine::glengine(void)
{
	debug_level = 0;
	shut=false;
	this->interface=NULL;
}


void glengine::addtocache(glproperty* given)
{
    if(given->context==GLOBAL){
      this->exports_cache.insert(pair<int,glproperty*>(given->index,given));
      this->imports_cache.insert(pair<int,glproperty*>(given->index,given));
    }
    if(given->context==EXPORT){
      this->exports_cache.insert(pair<int,glproperty*>(given->index,given));
    }
    if(given->context==IMPORT){
      this->imports_cache.insert(pair<int,glproperty*>(given->index,given));
    }
    pair<PROPOERTYCONTEXT,int> toindex(given->context,given->index);
    this->cacheIndex.insert(pair<string,pair<PROPOERTYCONTEXT,int> >(given->name,toindex));
}

glproperty* glengine::getFromCache(string name) const
{
    map<string,pair<PROPOERTYCONTEXT,int> >::const_iterator it=this->cacheIndex.find(name);
    if(it==this->cacheIndex.end())
      return NULL;
    int index=it->second.second;
    if(it->second.first==EXPORT || it->second.first==GLOBAL){
     map<int,glproperty*>::const_iterator it= this->exports_cache.find(index);
     if(it==this->exports_cache.end())
       return NULL;
     return it->second;
    }
    else{
     map<int,glproperty*>::const_iterator it= this->imports_cache.find(index);
     if(it==this->imports_cache.end())
       return NULL;
     return it->second;
    }
     
}

int glengine::getType(const string name, string& type) const
{
 glproperty *fc=getFromCache(name);
    if(fc==NULL)
      return -1;
    type = fc->getType();
    return 0;
}


int glengine::get(const string name, string& value) const
{
    glproperty *fc=getFromCache(name);
    if(fc==NULL)
      return -1;
    value=fc->getValue();
    return 0;
}

int glengine::get(const char* name, char* value, size_t len) const
{
    string toReturn;
    if(get(string(name),toReturn)!=0)
      return -1;
    
    memset(value,0,len);
    int cpylen = len < toReturn.length() ? len : toReturn.length();
    memcpy(value,toReturn.c_str(),cpylen);
    
    return 0;
}

int glengine::set(const char* name, const char* value)
{
  if(set(string(name),string(value))!=0)
    return -1;
  return 0;
}

int glengine::set(const string name, const string value)
{
    map<string,pair<PROPOERTYCONTEXT,int> >::const_iterator it=this->cacheIndex.find(name);
     
    if(it==this->cacheIndex.end())
      return -1;
    
    if(it->second.first==EXPORT)
     this->exports_cache[it->second.second]->value=value;
    else
    if(it->second.first==IMPORT)
      this->imports_cache[it->second.second]->value=value;
    else{
      this->exports_cache[it->second.second]->value=value;
      this->imports_cache[it->second.second]->value=value;
    }
    return 0;
}

void glengine::startGLD(const char *gldexecpath,const char **args){
	
	debug(1,"Starting GLD process");
#ifdef _WIN32
	stringstream ss;
	ss << gldexecpath << "\\" << "gridlabd.exe";
	int i=0;
	while(args[i]!=NULL)
		ss << " " << args[i++];
	string command=ss.str();
	STARTUPINFO glsi;
	ZeroMemory(&glsi, sizeof(glsi));
	glsi.cb=sizeof(glsi);
	ZeroMemory(&this->gldpid,sizeof(this->gldpid));
	LPSTR s=const_cast<char *>(command.c_str());
	if(!CreateProcess(NULL,s,NULL,NULL,FALSE,0,NULL,NULL,&glsi,&this->gldpid)){
		exception("staring gld failed");
	}
	debug(1,"GLD started");
#else
	string exec_string(gldexecpath);
	exec_string += "//gridlabd";
	vector<const char *> args;
	argv.push_back(exec_string.c_str());
	int i=0;
	while(args[i]!=NULL)
		argv.push_back(args[i++]);
	argv.push_back(NULL);
	this->gldpid=fork();
	if(gldpid<0)
		exception("fork gld process failed!")
	if(gldpid==0){ //child
		if(execv(argv[0],&argv[0])<0)
			exception("Executing gld failed, check gld path!");
	}else{
		debug(0,"Forked child process to start gld!");
		return;
	}
#endif
}

void glengine::protocolConnect(){

	unsigned int retries = 8;

	debug(1,"Trying to establish connection to GLD process");
	do{
		sleep(1000);
		try{
			if (! this->interface->connect()){
				retries--;
				continue;
			}
			if ( send("INIT")<=0 )
			{
				exception("send INIT failed ");
			}

			// read version response
			if (recv_gldversion() )
				break;
			else
				retries--;
		}catch(const char *){
			retries--;
		}
		
	}while(retries>0);
	if(retries==0){
		exception("Connection to gridlab-d failed!");
	}

	debug(1,"connection open with GridLAB-D Version %d.%d.%d-%d (%s)", major, minor, patch, build, branch);

	if ( !recvProtocol(protocol) ){
		exception("unexpected message [%s]", buffer);
	}

	if ( !recvCacheSize(cachesize) )
	{
		exception("unexpected message [%s]", buffer);
	}

	if ( !recvTimeout(timeout) )
	{	
		exception("unexpected message [%s]", buffer);
	}
	warning("Current verssion does not support timeouts.");

	debug(1,"reading exports and imports...");
	// read global data list
	char spec[1500];
	while ( recvProperty(spec))
	{
		  glproperty *prop=glproperty::deserialize(spec);
		  addtocache(prop);
		  debug(2,"data spec [%s]", spec);
	}

	if ( send("OK")<=0 )
	{
		exception("send status OK failed");
	}
	debug(1,"Connection established");
}

bool glengine::recvProtocol(char *given){

	if(recv(buffer,1500)<0)
		return false;
	if(sscanf(buffer,"PROTOCOL %s",given) !=1)
		return false;
	return true;

}

bool glengine::recvCacheSize(int &cacheSize){
	if(recv(buffer,1500)<0)
		return false;
	if(sscanf(buffer,"CACHESIZE %d",&cacheSize) !=1)
		return false;
	return true;
}

bool glengine::recvTimeout(int &timeout){
	if(recv(buffer,1500)<0)
		return false;
	if(sscanf(buffer,"TIMEOUT %d",&timeout) !=1)
		return false;
	return true;
}

bool glengine::recvProperty(char *prop){

	if(recv(buffer,1500)<0)
		return false;
	if(strcmp(buffer,"DONE")==0)
		return false;
	sscanf(buffer,"%1500[^\n]",prop);

	return true;
}
void glengine::connect(const char *pathToGLD,const char *parameters,...){
	if(pathToGLD==NULL)
		exception("PATH to GLS is not specified!");
	vector<const char *> argsVec;
	if ( parameters!=NULL )
	{
		va_list ptr;
		va_start(ptr,parameters);
		argsVec.push_back(parameters);
		char *arg;
		while ( (arg=va_arg(ptr,char*))!=NULL )
		{
			argsVec.push_back(arg);
		}
		va_end(ptr);
		argsVec.push_back(NULL);
		
	}
	else{
		argsVec.push_back(NULL);
	}
	this->startGLD(pathToGLD,&argsVec[0]);

	this->interface=absconnection::getconnection(UDP,string("127.0.0.1"),6267);
	if(interface==NULL)
		exception("Interface creation error, unsupported socket type or socket operation error");
	protocolConnect();
}

void glengine::connect(ENGINE_SOCK_TYPE socktype,const char *pathToGLD,const char *parameters,...){
	
	if(pathToGLD==NULL)
		exception("PATH to GLS is not specified!");
	vector<const char *> argsVec;
	if ( parameters!=NULL )
	{
		va_list ptr;
		va_start(ptr,parameters);
		argsVec.push_back(parameters);
		char *arg;
		while ( (arg=va_arg(ptr,char*))!=NULL )
		{
			argsVec.push_back(arg);
		}
		va_end(ptr);
		argsVec.push_back(NULL);
		
	}
	else{
		argsVec.push_back(NULL);
	}
	this->startGLD(pathToGLD,&argsVec[0]);

	this->interface=absconnection::getconnection(socktype,string("127.0.0.1"),6267);
	if(interface==NULL)
		exception("Interface creation error, unsupported socket type or socket operation error");

	protocolConnect();
}

void glengine::connect(ENGINE_SOCK_TYPE socktype,const string &ip,const int &port){
	this->interface=absconnection::getconnection(socktype,ip,port);
	if(interface==NULL)
		exception("Interface creation error, unsupported socket type or socket operation error");

	protocolConnect();
}

void glengine::sleep(unsigned int msec)
{
#ifdef _WIN32
	Sleep(msec);
#else
	usleep(msec*1000);
#endif
}


glengine::~glengine(void)
{
	if(!shut)
		shutdown(SIGTERM);
	delete this->interface;
}

int glengine::shutdown(const int signum)
{
	if(shut)
		return 0;
	shut=true;
	debug(1,"shutdown of engine requested");
#ifdef _WIN32
	
	if(TerminateProcess(this->gldpid.hProcess,0))
		WaitForSingleObject(this->gldpid.hProcess,INFINITE);
	CloseHandle(this->gldpid.hProcess);
	//CloseHandle(this->gldpid.hThread);
#else
	
	if(kill(this->gldpid,SIGTERM))
		wait(NULL);
#endif
	
	return 0;
}

int glengine::send(const char *fmt, ...)
{
	memset(buffer,0,1500);//safety!
	va_list ptr;
	va_start(ptr,fmt);
	size_t len = vsprintf(buffer,fmt,ptr);
	va_end(ptr);
	int slen=interface->send(buffer,len);
	if ( slen<0 )
		exception("unable to send message [%s] (errno=%s)", buffer, interface->getErrorMessage());
	return slen;
}


int glengine::recv(char *buffer, const int &maxLen,bool setTimeout){
	
	memset(buffer,0,maxLen);//safety!
	
	int len; 
	len = interface->recv(buffer,maxLen);
	assert(len <=maxLen);
	if(len < 0)
		return len;

	if(len==maxLen) //we overfilled the buffer, may lead to parse errors!
	   buffer[len-1]='\0';
	else
		buffer[len]='\0';
	debug(2,"receid %u byte message from engine, message '%s'", len,buffer);
	return len;
}

bool glengine::recv_gldversion(){
	
	recv(buffer,1500) ;
	
	if( sscanf(buffer,"GRIDLABD %d.%d.%d (%32[^)])",&major,&minor,&patch,branch) != 4)
		return false;
	return true;
}

bool glengine::recvTime(int64_t &syncTime){
	buffer[0]='\0';
	recv(buffer,1500);
	long toReturn;
	if( sscanf(buffer,"SYNC %ld",&syncTime) !=1)
		return false;
	return true;
}

bool glengine::recvSyncState(string &state){
	if(recv(buffer,1500) < 0)
		return false;

	state=string(buffer);
	return true;
}


int glengine::sync_start(int64_t &syncCurrentTime)
{
 

  debug(1,"Starting sycn set (sync_start())");

  
  if(!recvTime(syncCurrentTime)){
    exception("Synchronization error, cannot receive time from server");
  }
  debug(2,"Sync_start received time %ld",syncCurrentTime);
  
  recv_exports();
  debug(2,"Sync_start received exports",time);
  //GET OK or TERM from server
 
  string syncState;
  if(!recvSyncState(syncState)){
    exception("Synchronization error, cannot get sync state from server");
  }
  debug(2,"Sync_start received %s",syncState.c_str());
  if(syncState=="TERM")
    return 0;
  else
    if(syncState!="OK"){
		exception("Expecting OK got %s",syncState.c_str());
    }
    debug(1,"Sync_start done!");
   return 1;
}

vector<string> glengine::getExportNames() const{
	
	map<int,glproperty*>::const_iterator it=this->exports_cache.begin();
	vector<string> toreturn;
	for(;it!=exports_cache.end();++it){
		toreturn.push_back(it->second->getName());
	}

	return toreturn;
}

vector<string> glengine::getImportNames() const{
	
	map<int,glproperty*>::const_iterator it=this->imports_cache.begin();
	vector<string> toreturn;
	for(;it!=imports_cache.end();++it){
		toreturn.push_back(it->second->getName());
	}

	return toreturn;
}

void glengine::sync_finalize(const int64_t syncto)
{
	debug(2,"Starting Sync_finalize()");
	if(send("SYNC %ld",syncto)<=0){
	  exception("Sync send failed!");
	}
	debug(1,"Sync sending imports");

	send_imports();

	if(send("OK")<=0){
	  exception("Sync send failed!");
	}
	debug(2,"Sync_finalize finished()");
}




void glengine::send_imports()
{
  for(int i=0;i<imports_cache.size();i++){
  
    if(send("%d %s",imports_cache[i]->index,imports_cache[i]->value.c_str())<=0){
      exception("send of imports failed!");
    }
  }
}

bool glengine::recvExport(int &index,string &value){

	if(recv(buffer,1500) < 0)
		return false;

	char cvalue[1000];
	memset(cvalue,0,1000);
	if(sscanf(buffer,"%d %s",&index,cvalue) != 2)
		return false;

	value=string(cvalue);
	return true;
}

void glengine::recv_exports()
{
  int count=exports_cache.size();
  int index;
  string value;
 
  for(;count>0;count--){
	if(!recvExport(index,value))
		exception("Recv expoert error!");
    map<int,glproperty*>::iterator it=this->exports_cache.find(index);
    if(it==this->exports_cache.end()){
      exception("Index %d of export item is invalid",index);
    }
      this->exports_cache[index]->value=value;
  }
}

