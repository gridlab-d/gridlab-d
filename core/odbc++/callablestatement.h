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

#ifndef __ODBCXX_CALLABLESTATEMENT_H
#define __ODBCXX_CALLABLESTATEMENT_H

#include <odbc++/setup.h>
#include <odbc++/types.h>
#include <odbc++/preparedstatement.h>

namespace odbc {

  /** A prepared statement suited for stored procedure calls.
   *
   * A <code>CallableStatement</code> extends the functionality of a
   * <code>PreparedStatement</code>, by allowing output parameters.
   *
   * The ODBC escapes for calling stored procedures and functions
   * should be used. A procedure call is prepared like this:
   * <pre>
   * std::auto_ptr&lt;CallableStatement&gt; cstmt
   *    =std::auto_ptr&lt;CallableStatement&gt;(con-&gt;prepareCall
   *     ("{call my_procedure(?,?,?)}"));
   * </pre>
   *
   * And for a function call (a procedure that returns a value), the
   * following syntax should be used:
   * <pre>
   * std::auto_ptr&lt;CallableStatement&gt; cstmt
   *    =std::auto_ptr&lt;CallableStatement&gt;(con-&gt;prepareCall
   *     ("{?=call my_function(?,?)}"));
   * </pre>
   *
   *
   * All parameters in a <code>CallableStatement</code> are treated
   * as input/output parameters, unless they are registered as
   * output-only parameters with registerOutParameter(). Note that
   * output-only parameters must be registered with their proper
   * SQL type prior to executing a <code>CallableStatement</code>.
   */
  class ODBCXX_EXPORT CallableStatement : public PreparedStatement {
    friend class Connection;

  private:
    bool lastWasNull_;

  protected:
    CallableStatement(Connection* con,
                      SQLHSTMT hstmt,
                      const ODBCXX_STRING& sql,
                      int resultSetType,
                      int resultSetConcurrency);

  public:
    /** Destructor */
    virtual ~CallableStatement();

    /** Fetches a parameter as a double
     * @param idx The parameter index, starting at 1
     */
    double getDouble(int idx);

    /** Fetches a parameter as a bool
     * @param idx The parameter index, starting at 1
     */
    bool getBoolean(int idx);

    /** Fetches a parameter as a signed char
     * @param idx The parameter index, starting at 1
     */
    signed char getByte(int idx);

    /** Fetches a parameter as a Bytes object
     * @param idx The parameter index, starting at 1
     */
    ODBCXX_BYTES getBytes(int idx);

    /** Fetches a parameter as a Date
     * @param idx The parameter index, starting at 1
     */
    Date getDate(int idx);

    /** Fetches a parameter as a float
     * @param idx The parameter index, starting at 1
     */
    float getFloat(int idx);

    /** Fetches a parameter as an int
     * @param idx The parameter index, starting at 1
     */
    int getInt(int idx);

    /** Fetches a parameter as a Long
     * @param idx The parameter index, starting at 1
     */
    Long getLong(int idx);

    /** Fetches a parameter as a short
     * @param idx The parameter index, starting at 1
     */
    short getShort(int idx);

    /** Fetches a parameter as a string
     * @param idx The parameter index, starting at 1
     */
    ODBCXX_STRING getString(int idx);

    /** Fetches a parameter as a Time
     * @param idx The parameter index, starting at 1
     */
    Time getTime(int idx);

    /** Fetches a parameter as a Timestamp
     * @param idx The parameter index, starting at 1
     */
    Timestamp getTimestamp(int idx);

    /** Registers an output parameter
     * @param idx The parameter index, starting at 1
     * @param sqlType The SQL type of the parameter
     * @see Types
     */
    void registerOutParameter(int idx, int sqlType) {
      this->registerOutParameter(idx,sqlType,0);
    }

    /** Registers an output parameter with a given scale
     * @param idx The parameter index, starting at 1
     * @param sqlType The SQL type of the parameter
     * @param scale The scale of the parameter.
     * @see Types
     */
    void registerOutParameter(int idx, int sqlType, int scale);

    /** Registers an input only parameter
     * @param idx The parameter index, starting at 1
     */
    void registerInParameter(int idx);

    /** Returns true if the last fetched parameter was NULL */
    bool wasNull() {
      return lastWasNull_;
    }
  };


} // namespace odbc


#endif // __ODBCXX_CALLABLESTATEMENT_H
