/* tm_module_link.cpp
 *	Copyright (C) 2008 Battelle Memorial Institute
*/

#ifndef TM_MODULE_LINK_H
#define TM_MODULE_LINK_H

/**
 * 
 * C++ Functions to be used by modules for transactional memory support
 * 
 */

#include "transactionmanager.h"

template<typename T>
void scheduleSubtractTransaction(T* ptrToOldValue,T* ptrToNewValue,int count=1){

  SubtractTransaction<T> *rp=new SubtractTransaction<T>(ptrToOldValue,ptrToNewValue,count);
  getTransactionManager()->scheduleTranscation(rp);
}

template<typename T>
void scheduleAdditionTransaction(T* ptrToOldValue,T* ptrToNewValue,int count=1){

  AddTransaction<T> *rp=new AddTransaction<T>(ptrToOldValue,ptrToNewValue,count);
  getTransactionManager()->scheduleTranscation(rp);
}

template<typename T>
void scheduleReplaceTransaction(T* ptrToOldValue,T* ptrToNewValue,int count=1){
  
  ReplaceTransaction<T> *rp=new ReplaceTransaction<T>(ptrToOldValue,ptrToNewValue,count);
  getTransactionManager()->scheduleTranscation(rp);
}

inline void scheduleTranscation(AbsTransaction *given){
  getTransactionManager()->scheduleTranscation(given);
}

/*inline void scheduleReplaceTransaction(complex* ptrToOldValue, complex* ptrToNewValue, int count){
  ReplaceComplex *rp=new ReplaceComplex(ptrToOldValue,ptrToNewValue,count);
  getTransactionManager()->scheduleTranscation(rp);
}*/

//we need c functions

#endif
