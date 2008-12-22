/* 
   This file is part of libodbc++.
   
   Copyright (C) 1999-2000 Manush Dodunekov <manush@stendahls.net>
   
   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.
   
   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.
   
   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#ifndef __ODBCXX_THREADS_H
#define __ODBCXX_THREADS_H

#include <odbc++/setup.h>

#if defined(ODBCXX_ENABLE_THREADS)

#if !defined(WIN32)
# include <pthread.h>
#endif

namespace odbc {
  
  class ODBCXX_EXPORT Mutex {
  private:
#if !defined(WIN32)
    pthread_mutex_t mutex_;
#else
    CRITICAL_SECTION mutex_;
#endif

    Mutex(const Mutex&);
    Mutex& operator=(const Mutex&);

  public:
    explicit Mutex();
    ~Mutex();

    void lock();
    void unlock();
  };

  class ODBCXX_EXPORT Locker {
  private:
    Mutex& m_;
  public:
    Locker(Mutex& m)
      :m_(m) {
      m_.lock();
    }
    
    ~Locker() {
      m_.unlock();
    }
  };

} //namespace odbc

// macro used all over the place
#define ODBCXX_LOCKER(mut) odbc::Locker _locker(mut)

#else // !ODBCXX_ENABLE_THREADS

#define ODBCXX_LOCKER(mut) ((void)0)

#endif

#endif // __ODBCXX_THREADS_H
