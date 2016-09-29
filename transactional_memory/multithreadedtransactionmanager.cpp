/* multithreadedtranscationmanager.cpp
 *	Copyright (C) 2008 Battelle Memorial Institute
 */



#include "multithreadedtransactionmanager.h"
#include <assert.h>
#include <iostream>

using namespace std;

map<void*,vector<AbsTransaction*> > *MultiThreadedTranscationManager::mapptr=NULL;

map<void*,vector<AbsTransaction*> >::iterator MultiThreadedTranscationManager::it;

vector<thread_struct> MultiThreadedTranscationManager::thread_datas;
bool MultiThreadedTranscationManager::work=true;
pthread_mutex_t MultiThreadedTranscationManager::iteratorLock;
pthread_mutex_t MultiThreadedTranscationManager::mcondLock;
pthread_cond_t MultiThreadedTranscationManager::mcondition;
int MultiThreadedTranscationManager::finished_count=0;


void MultiThreadedTranscationManager::commitTransactions()
{
  finished_count=0;
  it=this->transactionMap.begin();
  if(it!=transactionMap.end()){
      cout << "Signalling commits!" << endl;
      signalAll();
      cout << "waiting commits" << endl;
      
      while(true){
	pthread_mutex_lock(&mcondLock);
	if(finished_count==thread_datas.size()){
	  pthread_mutex_unlock(&mcondLock);
	  break;
	}
	pthread_cond_wait(&mcondition,&mcondLock);
	pthread_mutex_unlock(&mcondLock);
      }
  }
  
}

void MultiThreadedTranscationManager::signalAll(bool doJoin)
{
  vector<thread_struct>::iterator it3=thread_datas.begin();
 
  for(;it3!=thread_datas.end();++it3){
    pthread_cond_signal(&((*it3).condition));
    if(doJoin)
      pthread_join((*it3).thread,NULL);
  }
}


MultiThreadedTranscationManager::~MultiThreadedTranscationManager()
{
  MultiThreadedTranscationManager::work=false;
  cout << "sending signalss!!!" << endl;
  signalAll(true);
  delete this->ids;
  cout << "done!!" << endl;
}


void* MultiThreadedTranscationManager::worker(void* context)
{
  int myId=*((int *)context);SYNOPSIS

#include <pthread.h> 

int pthread_join(pthread_t thread, void **retval);


Compile and link with -pthread.
DESCRI
  bool dowork=true;
  map<void*,vector<AbsTransaction*> >::iterator internalIterator;
  while(dowork){
    cout << "Thread " << myId << " entering cond wait" << endl;
    pthread_mutex_lock(&MultiThreadedTranscationManager::thread_datas[myId].condLock);
    cout << "Thread " << myId << " accuquired cond lock" << endl;
    pthread_cond_wait(&MultiThreadedTranscationManager::thread_datas[myId].condition,&MultiThreadedTranscationManager::thread_datas[myId].condLock);
      dowork=MultiThreadedTranscationManager::work;
    pthread_mutex_unlock(&MultiThreadedTranscationManager::thread_datas[myId].condLock);
    cout << "Thread " << myId << " awake!!" << endl;
    if(!dowork)
      break;
    while(true){
      pthread_mutex_lock(&iteratorLock);
	  cout << "Thread " << myId << " got the lock on iterator!" << endl;
	  internalIterator=it;
	  if(it!=MultiThreadedTranscationManager::mapptr->end())
	    ++it;
      pthread_mutex_unlock(&iteratorLock);
      if(internalIterator==MultiThreadedTranscationManager::mapptr->end())
	break;
      void *valPtr=internalIterator->first;
     
      for(vector<AbsTransaction*>::iterator vit=internalIterator->second.begin();
	    vit!=internalIterator->second.end();++vit){
	(*vit)->executeTransaction();
	delete (*vit);
	(*vit)=NULL;
        
      }
    }
    cout << "Thread " << myId << "Finished working" << endl;
    finished_count++; //can fail!
    pthread_cond_signal(&mcondition);
  }
  return NULL;
}

MultiThreadedTranscationManager::MultiThreadedTranscationManager(int threadCount) : TransactionManager()
{
  //pthread_mutex_init(&MultiThreadedTranscationManager::condLock);
  mapptr=&this->transactionMap;
  this->ids=new int[threadCount];
  thread_struct aThread_Data;
  aThread_Data.condLock=PTHREAD_MUTEX_INITIALIZER;
  aThread_Data.condition=PTHREAD_COND_INITIALIZER;
  
  for(int i=0;i<threadCount;i++){
    ids[i]=i;
    pthread_create(&aThread_Data.thread,NULL,MultiThreadedTranscationManager::worker,&ids[i]);
    thread_datas.push_back(aThread_Data);
    
  }
  pthread_mutex_init(&MultiThreadedTranscationManager::iteratorLock,NULL);
  
}


