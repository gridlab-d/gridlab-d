/* transactionmanager.cpp
 *	Copyright (C) 2008 Battelle Memorial Institute
 */


#include "transactionmanager.h"

TransactionManager* TransactionManager::instance=NULL;

TransactionManager::TransactionManager()
{
  pthread_mutex_init(&mutex,NULL);
}

TransactionManager *getTransactionManager(){
  return TransactionManager::instance;
};


void TransactionManager::scheduleTranscation(AbsTransaction *given)
{
  void *key=given->getOldPtr();
  //the code below saves from two O(logN) calls.
  pthread_mutex_lock(&mutex);
  pair<map<void*,vector<AbsTransaction*> >::iterator,bool> result=
      transactionMap.insert(pair<void*,vector<AbsTransaction*> >(key,vector<AbsTransaction*>()));
 
   result.first->second.push_back(given);
  pthread_mutex_unlock(&mutex);
}

void TransactionManager::commitTransaction(AbsTransaction *given){
	given->executeTransaction();
	delete given;
}

TransactionManager::~TransactionManager(){

	//commit the transactions.
	//this->commit();
	this->transactionMap.clear(); //leak!

}

void TransactionManager::commit()
{
  this->commitTransactions();
  this->transactionMap.clear();
}

SingleThreadedTransactionManager::SingleThreadedTransactionManager() : TransactionManager(){

}


void SingleThreadedTransactionManager::commitTransactions()
{
  map<void*,vector<AbsTransaction*> >::iterator it=this->transactionMap.begin();
  
  for(;it!=transactionMap.end();++it){

      for(vector<AbsTransaction*>::iterator vit=it->second.begin();
	    vit!=it->second.end();++vit){
		commitTransaction(*vit);
		(*vit)=NULL;
      }
  }
  
}



