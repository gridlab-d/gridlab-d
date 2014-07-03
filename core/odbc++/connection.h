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

#ifndef __ODBCXX_CONNECTION_H
#define __ODBCXX_CONNECTION_H

#include <odbc++/setup.h>
#include <odbc++/types.h>
#include <odbc++/errorhandler.h>

namespace odbc {

  class DriverInfo;
  class DatabaseMetaData;
  class Statement;
  class PreparedStatement;
  class CallableStatement;

  /** A database connection */
  class ODBCXX_EXPORT Connection : public ErrorHandler {
    friend class DriverManager;
    friend class Statement;
    friend class DatabaseMetaData;
    friend class DriverInfo;

  private:
    struct PD;
    // private data
    PD* pd_;

    SQLHDBC hdbc_;

    DatabaseMetaData* metaData_;
    DriverInfo* driverInfo_;

    //utilities
    SQLUINTEGER _getNumericOption(SQLINTEGER optnum);
    ODBCXX_STRING _getStringOption(SQLINTEGER optnum);
    void _setNumericOption(SQLINTEGER optnum, SQLUINTEGER value);
    void _setStringOption(SQLINTEGER optnum, const ODBCXX_STRING& value);

    SQLHSTMT _allocStmt();

    //private constructor, called from DriverManager
    Connection(SQLHDBC h);

    void _connect(const ODBCXX_STRING& dsn,
                  const ODBCXX_STRING& user,
                  const ODBCXX_STRING& password);

    void _connect(const ODBCXX_STRING& connectString);

    void _registerStatement(Statement* stmt);
    void _unregisterStatement(Statement* stmt);

    const DriverInfo* _getDriverInfo() const {
      return driverInfo_;
    }

  public:
    /** Transaction isolation constants.
     */
    enum TransactionIsolation {
      /** The data source does not support transactions */
      TRANSACTION_NONE,
      /** Dirty reads, non-repeatable reads and phantom reads can occur. */
      TRANSACTION_READ_UNCOMMITTED,
      /** Non-repeatable and phantom reads can occur */
      TRANSACTION_READ_COMMITTED,
      /** Phantom reads can occur */
      TRANSACTION_REPEATABLE_READ,
      /** Simply no problems */
      TRANSACTION_SERIALIZABLE
    };

    /** Destructor. Closes the connection */
    virtual ~Connection();

    /** Returns true if autocommit is on */
    bool getAutoCommit();

    /** Sets the autocommit state of this connection
     * @param autoCommit <tt>true</tt> for on, <tt>false</tt> for off
     */
    void setAutoCommit(bool autoCommit);

    /** Commits the ongoing transaction */
    void commit();

    /** Rollbacks the ongoing transaction */
    void rollback();

    /** Returns the current catalog */
    ODBCXX_STRING getCatalog();

    /** Sets the current catalog */
    void setCatalog(const ODBCXX_STRING& catalog);

    /** Returns the current transaction isolation level */
    TransactionIsolation getTransactionIsolation();

    /** Sets the current transaction isolation level */
    void setTransactionIsolation(TransactionIsolation isolation);

    /** Returns true if the connection is read only */
    bool isReadOnly();

    /** Sets the read-only state of this connection */
    void setReadOnly(bool readOnly);

    /** Returns true if ODBC tracing is enabled on this connection
     */
    bool getTrace();

    /** Sets ODBC tracing on or off */
    void setTrace(bool on);

    /** Returns the file ODBC tracing is currently written to */
    ODBCXX_STRING getTraceFile();

    /** Sets the file ODBC tracing is written to */
    void setTraceFile(const ODBCXX_STRING& s);

    /** Returns meta information for this connection.
     *
     * Note that the returned object is 'owned' by this
     * connection and should in no way be deleted by the caller.
     *
     * Example:
     * <tt>DatabaseMetaData* dmd = cnt-&gt;getMetaData();</tt>
     */
    DatabaseMetaData* getMetaData();


    /** Creates a non-prepared statement.
     *
     * Example:
     * <tt>std::auto_ptr&lt;Statement&gt; stmt =
     *     std::auto_ptr&lt;Statement&gt;(cnt-&gt;createStatement());</tt>
     */
    Statement* createStatement();

    /** Creates a non-prepared statement.
     *
     * Example:
     * <tt>std::auto_ptr&lt;Statement&gt; stmt =
     *     std::auto_ptr&lt;Statement&gt;(cnt-&gt;createStatement(x, y));</tt>
     *
     * @param resultSetType The type for <tt>ResultSet</tt>s created
     * by this statement
     * @param resultSetConcurrency The concurrency for <tt>ResultSet</tt>s created
     * by this statement
     */
    Statement* createStatement(int resultSetType,
                               int resultSetConcurrency);


    /** Create a prepared statement.
     *
     * Example:
     * <tt>std::auto_ptr&lt;PreparedStatement&gt; pstmt =
     *     std::auto_ptr&lt;PreparedStatement&gt;(cnt-&gt;prepareStatement(s));</tt>
     *
     * @param sql The string to prepare, optionally containing parameter
     * markers (<tt>?</tt>).
     */
    PreparedStatement* prepareStatement(const ODBCXX_STRING& sql);

    /** Create a prepared statement.
     *
     * Example:
     * <tt>std::auto_ptr&lt;PreparedStatement&gt; pstmt =
     *     std::auto_ptr&lt;PreparedStatement&gt;(cnt-&gt;prepareStatement(s, x, y));</tt>
     *
     * @param sql The string to prepare, optionally containing parameter
     * markers.
     * @param resultSetType The type for <tt>ResultSet</tt>s created
     * by this statement
     * @param resultSetConcurrency The concurrency for <tt>ResultSet</tt>s created
     * by this statement
     */
    PreparedStatement* prepareStatement(const ODBCXX_STRING& sql,
                                        int resultSetType,
                                        int resultSetConcurrency);

    /** Create a callable prepared statement.
     *
     * Example:
     * <tt>std::auto_ptr&lt;CallableStatement&gt; cstmt =
     *     std::auto_ptr&lt;CallableStatement&gt;(cnt-&gt;prepareCall(s));</tt>
     *
     * @param sql The string to prepare, optionally containing parameter
     * markers for input and/or output parameters
     */
    CallableStatement* prepareCall(const ODBCXX_STRING& sql);

    /** Create a callable prepared statement.
     *
     * Example:
     * <tt>std::auto_ptr&lt;CallableStatement&gt; cstmt =
     *     std::auto_ptr&lt;CallableStatement&gt;(cnt-&gt;prepareCall(s, x, y));</tt>
     *
     * @param sql The string to prepare, optionally containing parameter
     * markers for input and/or output parameters
     * @param resultSetType The type for <tt>ResultSet</tt>s created
     * by this statement
     * @param resultSetConcurrency The concurrency for <tt>ResultSet</tt>s created
     * by this statement
     */
    CallableStatement* prepareCall(const ODBCXX_STRING& sql,
                                   int resultSetType,
                                   int resultSetConcurrency);

    /** Translate a given SQL string into this data sources' own
     * SQL grammar.
     */
    ODBCXX_STRING nativeSQL(const ODBCXX_STRING& sql);
  };



} // namespace odbc


#endif // __ODBCXX_CONNECTION_H
