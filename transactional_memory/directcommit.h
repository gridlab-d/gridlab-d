#ifndef DIRECTCOMMIT_H
#define DIRECTCOMMIT_H

#include "transactionmanager.h"
#include "tm_core_link.h"


class DirectCommit : public TransactionManager
{
  friend TransactionManager *getTransactionManager();
  friend void initTransactionManager(int numberofThreads);
  protected:
      DirectCommit();
      virtual void commitTransactions();
  public:
      ~DirectCommit();
      virtual void scheduleTranscation(AbsTransaction* given);
      
};

#endif // DIRECTCOMMIT_H
