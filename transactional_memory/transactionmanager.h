/* transactionmanager.h
 *	Copyright (C) 2008 Battelle Memorial Institute
 */



#ifndef TRANSACTIONMANAGER_H
#define TRANSACTIONMANAGER_H

#include <map>
#include <vector>
#include "transaction.h"
#include "tm_core_link.h"
#include <pthread.h>

using namespace std;
//EXPERIMENTAL!!!!
class TransactionManager;

TransactionManager *getTransactionManager();

class TransactionManager
{
  friend TransactionManager *getTransactionManager();
  friend void initTransactionManager(int numberofThreads);
  protected:
    map<void*,vector<AbsTransaction*> > transactionMap;
    static TransactionManager* instance;
    pthread_mutex_t mutex;
    TransactionManager();
    /**
     * Internal commitTransaction method.
     * Subclasses implement the algorithm for commiting all transactions.
     * To commit a transaction call commitTransaction method This
     * will delete the transaction.
     */
    virtual void commitTransactions() =0;

    /**
     * This call executes the given transaction and deletes it!
     */
    void commitTransaction(AbsTransaction *given);
  public:
    /**
     * Schedules a transaction.
     */
    virtual void scheduleTranscation(AbsTransaction *given);
    virtual ~TransactionManager();
   
    /**
     * Commits transactions to the memory.
     *
     */
    void commit();
};

class SingleThreadedTransactionManager: public TransactionManager{
  public:
		 SingleThreadedTransactionManager();
  protected:
    virtual void commitTransactions();

};



#endif // TRANSACTIONMANAGER_H
