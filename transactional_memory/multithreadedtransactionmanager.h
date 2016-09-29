/* transaction.h
 *	Copyright (C) 2008 Battelle Memorial Institute
 */


#ifndef MULTITHREADEDTRANSCATIONMANAGER_H
#define MULTITHREADEDTRANSCATIONMANAGER_H

#include "transactionmanager.h"

#include <map>
#include <vector>
#include <pthread.h>

using namespace std;

struct thread_struct{
  pthread_t thread;
  pthread_cond_t condition;
  pthread_mutex_t condLock;
};

/**
 * Multithreaded transaction manager. Uses multiple threads to commit the transactions.
 *This code is not tested yet. 
 */
class MultiThreadedTranscationManager : public TransactionManager
{
  friend void initManager(int numberofThreads);
  private:
   static map<void*,vector<AbsTransaction*> >::iterator it;
   static map<void*,vector<AbsTransaction*> > *mapptr;
   void signalAll(bool doJoin=false);
   static void* worker(void *context);
   static pthread_mutex_t iteratorLock,mcondLock;
   static vector<thread_struct> thread_datas;
   static pthread_cond_t mcondition;
   static bool work;
   static int finished_count;
   int *ids;
  protected:
    virtual void commitTransactions();
  public:
    MultiThreadedTranscationManager(int threadCount=2);
    virtual ~MultiThreadedTranscationManager();
};

#endif // MULTITHREADEDTRANSCATIONMANAGER_H
