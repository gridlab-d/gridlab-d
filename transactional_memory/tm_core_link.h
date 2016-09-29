/* tm_core_link.h
 *	Copyright (C) 2008 Battelle Memorial Institute
 */

#ifndef TM_CORE_LINK_H_
#define TM_CORE_LINK_H_

#ifdef __cplusplus
extern "C"{
#endif

/**
 * Functions to be used by core for transactional memort support
 * 
 */

void initTransactionManager(int numberofThreads);
void commitTransactions();

#ifdef __cplusplus
}
#endif

#endif