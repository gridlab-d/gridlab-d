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

#ifndef __ODBCXX_ERRORHANDLER_H
#define __ODBCXX_ERRORHANDLER_H

#include <odbc++/setup.h>
#include <odbc++/types.h>
#include <odbc++/threads.h>

namespace odbc {

  /** Base class for everything that might contain warnings */
  class ODBCXX_EXPORT ErrorHandler {
    friend class DriverManager;
    friend class DataStreamBuf;
    friend class DataStream;
  private:
    // private data
    struct PD;
    PD* pd_;

    WarningList* warnings_;
    bool collectWarnings_;

    //the maxumum number of warnings to contain at a time
    enum {
      MAX_WARNINGS=128
    };

  protected:
    void _postWarning(SQLWarning* w);


#if ODBCVER < 0x0300
    void _checkErrorODBC2(SQLHENV henv,
			  SQLHDBC hdbc,
			  SQLHSTMT hstmt,
			  SQLRETURN r,
			  const ODBCXX_STRING& what);
#else

    void _checkErrorODBC3(SQLINTEGER handleType,
			  SQLHANDLE h,
			  SQLRETURN r, const ODBCXX_STRING& what);
#endif //ODBCVER < 0x0300

    void _checkStmtError(SQLHSTMT hstmt,
			 SQLRETURN r, const ODBCXX_CHAR_TYPE* what=ODBCXX_STRING_CONST("")) {

      if(r==SQL_SUCCESS_WITH_INFO || r==SQL_ERROR) {
#if ODBCVER < 0x0300

	this->_checkErrorODBC2(SQL_NULL_HENV, SQL_NULL_HDBC, hstmt,
			       r,ODBCXX_STRING_C(what));
#else
	
	this->_checkErrorODBC3(SQL_HANDLE_STMT,hstmt,r,ODBCXX_STRING_C(what));
	
#endif
      }
    }

    void _checkConError(SQLHDBC hdbc,
                        SQLRETURN r,
                        const ODBCXX_CHAR_TYPE* what=ODBCXX_STRING_CONST("")) {
      if(r==SQL_SUCCESS_WITH_INFO || r==SQL_ERROR) {
#if ODBCVER < 0x0300
	
	this->_checkErrorODBC2(SQL_NULL_HENV, hdbc, SQL_NULL_HSTMT, r,
			       ODBCXX_STRING_C(what));
	
#else
	
	this->_checkErrorODBC3(SQL_HANDLE_DBC, hdbc, r, ODBCXX_STRING_C(what));

#endif
      }
    }

    void _checkEnvError(SQLHENV henv,
                        SQLRETURN r,
                        const ODBCXX_CHAR_TYPE* what=ODBCXX_STRING_CONST("")) {
      if(r==SQL_SUCCESS_WITH_INFO || r==SQL_ERROR) {
#if ODBCVER < 0x0300
	
	this->_checkErrorODBC2(henv,SQL_NULL_HDBC,SQL_NULL_HSTMT,r,
			       ODBCXX_STRING_C(what));
	
#else
	
	this->_checkErrorODBC3(SQL_HANDLE_ENV,henv,r,ODBCXX_STRING_C(what));
	
#endif
      }
    }

    /** Constructor */
    ErrorHandler(bool collectWarnings =true);

  public:
    /** Clears all the warnings stored in this object */
    void clearWarnings();

    /** Fetches all the warnings in this object.
     * The caller is responsive for deleteing the
     * returned object.
     */
    WarningList* getWarnings();

    /** Destructor */
    virtual ~ErrorHandler();
  };


} // namespace odbc

#endif
