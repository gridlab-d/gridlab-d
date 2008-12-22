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

#ifndef __ODBCXX_STATEMENT_H
#define __ODBCXX_STATEMENT_H

#include <odbc++/setup.h>
#include <odbc++/types.h>
#include <odbc++/errorhandler.h>
#include <odbc++/connection.h>

namespace odbc {

  class ResultSet;
  class DriverInfo;

  /** A simple non-prepared statement */
  class ODBCXX_EXPORT Statement : public ErrorHandler {
    friend class Connection;
    friend class ResultSet;
    friend class DatabaseMetaData;

  protected:
    Connection* connection_;
    SQLHSTMT hstmt_;
    int lastExecute_;

    const DriverInfo* _getDriverInfo() const {
      return connection_->_getDriverInfo();
    }

  private:
    ResultSet* currentResultSet_;

    int fetchSize_;
    int resultSetType_;
    int resultSetConcurrency_;

    //used internally
    enum StatementState {
      STATE_CLOSED,
      STATE_OPEN
    };

    StatementState state_;

    std::vector<ODBCXX_STRING> batches_;

    void _registerResultSet(ResultSet* rs);
    void _unregisterResultSet(ResultSet* rs);

    void _applyResultSetType();

    ResultSet* _getTypeInfo();
    ResultSet* _getTables(const ODBCXX_STRING& catalog,
			  const ODBCXX_STRING& schema,
			  const ODBCXX_STRING& tableName,
			  const ODBCXX_STRING& types);

    ResultSet* _getTablePrivileges(const ODBCXX_STRING& catalog,
				   const ODBCXX_STRING& schema,
				   const ODBCXX_STRING& tableName);

    ResultSet* _getColumnPrivileges(const ODBCXX_STRING& catalog,
				    const ODBCXX_STRING& schema,
				    const ODBCXX_STRING& tableName,
				    const ODBCXX_STRING& columnName);

    ResultSet* _getPrimaryKeys(const ODBCXX_STRING& catalog,
			       const ODBCXX_STRING& schema,
			       const ODBCXX_STRING& tableName);

    ResultSet* _getColumns(const ODBCXX_STRING& catalog,
			   const ODBCXX_STRING& schema,
			   const ODBCXX_STRING& tableName,
			   const ODBCXX_STRING& columnName);

    ResultSet* _getIndexInfo(const ODBCXX_STRING& catalog,
			     const ODBCXX_STRING& schema,
			     const ODBCXX_STRING& tableName,
			     bool unique, bool approximate);

    ResultSet* _getCrossReference(const ODBCXX_STRING& pc,
				  const ODBCXX_STRING& ps,
				  const ODBCXX_STRING& pt,
				  const ODBCXX_STRING& fc,
				  const ODBCXX_STRING& fs,
				  const ODBCXX_STRING& ft);


    ResultSet* _getProcedures(const ODBCXX_STRING& catalog,
			      const ODBCXX_STRING& schema,
			      const ODBCXX_STRING& procName);

    ResultSet* _getProcedureColumns(const ODBCXX_STRING& catalog,
				    const ODBCXX_STRING& schema,
				    const ODBCXX_STRING& procName,
				    const ODBCXX_STRING& colName);

    ResultSet* _getSpecialColumns(const ODBCXX_STRING& catalog,
				  const ODBCXX_STRING& schema,
				  const ODBCXX_STRING& table,
				  int what,int scope,int nullable);

  protected:
    Statement(Connection* con, SQLHSTMT hstmt,
	      int resultSetType, int resultSetConcurrency);

    //utilities
    SQLUINTEGER _getNumericOption(SQLINTEGER optnum);
    ODBCXX_STRING _getStringOption(SQLINTEGER optnum);

    void _setNumericOption(SQLINTEGER optnum, SQLUINTEGER value);
    void _setStringOption(SQLINTEGER optnum, const ODBCXX_STRING& value);

#if ODBCVER >= 0x0300
    SQLPOINTER _getPointerOption(SQLINTEGER optnum);
    void _setPointerOption(SQLINTEGER optnum, SQLPOINTER value);
#endif

    //this returns true if we have a result set pending
    bool _checkForResults();

    //this _always_ returns a ResultSet. If hideMe is true, this statement
    //becomes 'owned' by the ResultSet
    ResultSet* _getResultSet(bool hideMe =false);

    //this is called before a Statement (or any of the derived classes)
    //is executed
    void _beforeExecute();

    //this is called after a successeful execution
    void _afterExecute();


  public:
    /** Destructor. Destroys/closes this statement as well as
     * all created resultsets.
     */
    virtual ~Statement();

    /** Returns the connection that created this statement */
    Connection* getConnection();


    /** Cancel an ongoing operation that was executed in another thread */
    void cancel();

    /** Execute a given SQL statement.
     * The statement can return multiple results. To get to the
     * next result after processing the first one, getMoreResults() should
     * be called.
     * @param sql The string to execute
     * @return true if a resultset is available
     */
    virtual bool execute(const ODBCXX_STRING& sql);

    /** Execute an SQL statement, expected to return a resultset.
     *
     * Example:
     * <tt>std::auto_ptr&lt;ResultSet&gt; rs =
     *     std::auto_ptr&lt;ResultSet&gt;(stmt-&gt;executeQuery(s));</tt>
     *
     * @param sql The string to execute
     * @return A ResultSet object.
     */
    virtual ResultSet* executeQuery(const ODBCXX_STRING& sql);

    /** Execute an SQL statement, expected to return an update count.
     * @return The number of affected rows
     */
    virtual int executeUpdate(const ODBCXX_STRING& sql);

    /** Fetch the current result as an update count.
     *
     * @return the current result's update count (affected rows), or <code>-1</code>
     * if the result is a ResultSet or if there are no more results.
     */
    int getUpdateCount();

    /** Fetch the current result as a ResultSet */
    ResultSet* getResultSet();

    /** Check if there are more results available on this
     * statment.
     * @return True if this statement has more results to offer.
     */
    bool getMoreResults();

    /** Set the cursor name for this statement */
    void setCursorName(const ODBCXX_STRING& name);

    /** Fetch the current fetch size (also called rowset size) for
     * resultsets created by this statement.
     */
    int getFetchSize() {
      return fetchSize_;
    }

    /** Set the current fetch size for resultsets created by this statement */
    void setFetchSize(int size);

    /** Get the concurrency type for resultsets created by this statement */
    int getResultSetConcurrency() {
      return resultSetConcurrency_;
    }

    /** Get the type for resultsets created by this statement */
    int getResultSetType() {
      return resultSetType_;
    }

    /** Get the query timeout for this statement */
    int getQueryTimeout();
    /** Set the query timeout for this statement */
    void setQueryTimeout(int seconds);

    /** Get the maximum number of rows to return in a resultset */
    int getMaxRows();
    /** Set the maximum number of rows to return in a resultset */
    void setMaxRows(int maxRows);

    /** Get the maximum field size for resultsets create by this statement */
    int getMaxFieldSize();
    /** Set the maximum field size for resultsets create by this statement */
    void setMaxFieldSize(int maxFieldSize);

    /** Sets escape processing on or off
     *
     * For <code>PreparedStatement</code>s, the command has been parsed on
     * creation, so this setting won't really have any effect.
     */
    void setEscapeProcessing(bool on);

    /** Gets the current escape processing setting
     * @return <code>true</code> if escape processing is on, <code>false</code>
     * otherwise
     */
    bool getEscapeProcessing();

    /** Closes all result sets from this execution. This is useful if
     * you don't wish to iterate through all remaining results, or if
     * your driver does not auto-close cursors. */
    void close();

  };



} // namespace odbc


#endif // __ODBCXX_STATEMENT_H
