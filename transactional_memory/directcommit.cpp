#include "directcommit.h"

DirectCommit::DirectCommit() : TransactionManager()
{

}

void DirectCommit::commitTransactions()
{
  //we do not do anything here, because schedule functions already commit the data
}

void DirectCommit::scheduleTranscation(AbsTransaction* given)
{
  given->executeTransaction();
}


DirectCommit::~DirectCommit()
{

}


