/* tm_core_link.cpp
 *	Copyright (C) 2008 Battelle Memorial Institute
 */

#include "tm_core_link.h"

#include "transactionmanager.h"
#include "directcommit.h"

void initTransactionManager(int numberofThreads){
    if(numberofThreads==0 || numberofThreads==1){

      TransactionManager::instance=new DirectCommit();
    }else{
      TransactionManager::instance=new SingleThreadedTransactionManager();
    }
    //add multithreaded later on
}

void commitTransactions(){
  
  getTransactionManager()->commit();
}
