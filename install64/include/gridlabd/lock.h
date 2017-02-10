/** $Id: lock.h 4738 2014-07-03 00:55:39Z dchassin $
 @{
 **/

#ifndef _LOCK_H
#define _LOCK_H

#ifdef __cplusplus
extern "C" {
#endif

void rlock(unsigned int *lock);
void wlock(unsigned int *lock);
void runlock(unsigned int *lock);
void wunlock(unsigned int *lock);

void register_lock(const char *name, unsigned int *lock);

#ifdef __cplusplus
}
#endif

#endif /* _LOCK_H */

/**@}**/

