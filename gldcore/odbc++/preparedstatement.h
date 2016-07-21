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


#ifndef __ODBCXX_PREPAREDSTATEMENT_H
#define __ODBCXX_PREPAREDSTATEMENT_H

#include <odbc++/setup.h>
#include <odbc++/types.h>
#include <odbc++/statement.h>

#if defined(ODBCXX_HAVE_ISO_CXXLIB)
# include <iosfwd>
#else
# include <iostream>
#endif

namespace odbc {

  class Rowset;

  /** A prepared statement.
   *
   * A prepared statement is precompiled by the driver and/or datasource,
   * and can be executed multiple times with different parameters.
   *
   * Parameters are set using the setXXX methods. Note that it's advisable
   * to use the set method compatible with the parameter's SQL type - for
   * example, for a <code>Types::DATE</code>, <code>setDate()</code> should
   * be used.
   * Question marks (<code>"?"</code>) are used in the SQL statement
   * to represent a parameter, for example:
   * <pre>
   * std::auto_ptr&lt;PreparedStatement&gt; pstmt
   *    =std::auto_ptr&lt;PreparedStatement&gt;(con-&gt;prepareStatement
   *    ("INSERT INTO SOMETABLE(AN_INTEGER_COL,A_VARCHAR_COL) VALUES(?,?)"));
   * pstmt->setInt(1,10);
   * pstmt->setString(2,"Hello, world!");
   * int affectedRows=pstmt-&gt;executeUpdate();
   * </pre>
   * @see Connection::prepareStatement()
   */

  class ODBCXX_EXPORT PreparedStatement : public Statement {
    friend class Connection;

  private:
    void _prepare();
    void _setupParams();

  protected:
    ODBCXX_STRING sql_;
    //here we store the parameters
    Rowset* rowset_;
    size_t numParams_;
    std::vector<int> directions_;
    int defaultDirection_;
    bool paramsBound_;

    PreparedStatement(Connection* con,
                      SQLHSTMT hstmt,
                      const ODBCXX_STRING& sql,
                      int resultSetType,
                      int resultSetConcurrency,
                      int defaultDirection =SQL_PARAM_INPUT);

    void _bindParams();
    void _unbindParams();

    void _checkParam(int idx,
                     int* allowed, int numAllowed,
                     int defPrec, int defScale);

  public:
    /** Destructor */
    virtual ~PreparedStatement();

    /** Clears the parameters.
     *
     * The set of parameters stays around until they are set again.
     * To explicitly clear them (and thus release buffers held by the
     * driver), this method should be called.
     */
    void clearParameters();

    /** Executes this statement.
     * @return True if the result is a ResultSet, false if it's an
     * update count or unknown.
     */
    bool execute();

    /** Executes this statement, assuming it returns a ResultSet.
     *
     * Example:
     * <tt>std::auto_ptr&lt;ResultSet&gt; rs =
     *     std::auto_ptr&lt;ResultSet&gt;(pstmt-&gt;executeQuery(s));</tt>
     *
     */
    ResultSet* executeQuery();

    /** Executes this statement, assuming it returns an update count */
    int executeUpdate();

    /** Sets a parameter value to a double
     * @param idx The parameter index, starting at 1
     * @param val The value to set
     */
    void setDouble(int idx, double val);

    /** Sets a parameter value to a bool
     * @param idx The parameter index, starting at 1
     * @param val The value to set
     */
    void setBoolean(int idx, bool val);

    /** Sets a parameter value to signed char
     * @param idx The parameter index, starting at 1
     * @param val The value to set
     */
    void setByte(int idx, signed char val);


    /** Sets a parameter value to a chunk of bytes
     * @param idx The parameter index, starting at 1
     * @param val The value to set
     */
    void setBytes(int idx, const ODBCXX_BYTES& val);

    /** Sets a parameter value to a Date
     * @param idx The parameter index, starting at 1
     * @param val The value to set
     */
    void setDate(int idx, const Date& val);

    /** Sets a parameter value to a float
     * @param idx The parameter index, starting at 1
     * @param val The value to set
     */
    void setFloat(int idx, float val);


    /** Sets a parameter value to an int
     * @param idx The parameter index, starting at 1
     * @param val The value to set
     */
    void setInt(int idx, int val);

    /** Sets a parameter value to a Long
     * @param idx The parameter index, starting at 1
     * @param val The value to set
     */
    void setLong(int idx, Long val);

    /** Sets a parameter value to a short
     * @param idx The parameter index, starting at 1
     * @param val The value to set
     */
    void setShort(int idx, short val);

    /** Sets a parameter value to a string
     * @param idx The parameter index, starting at 1
     * @param val The value to set
     */
    void setString(int idx, const ODBCXX_STRING& val);

    /** Sets a parameter value to a Time
     * @param idx The parameter index, starting at 1
     * @param val The value to set
     */
    void setTime(int idx, const Time& val);

    /** Sets a parameter value to a Timestamp
     * @param idx The parameter index, starting at 1
     * @param val The value to set
     */
    void setTimestamp(int idx, const Timestamp& val);

    /** Sets a parameter value to an ascii stream.
     * @param idx The parameter index, starting at 1
     * @param s The stream to assign
     * @param len The number of bytes available in the stream
     */
    void setAsciiStream(int idx, ODBCXX_STREAM* s, int len);

    /** Sets a parameter value to a binary stream.
     * @param idx The parameter index, starting at 1
     * @param s The stream to assign
     * @param len The number of bytes available in the stream
     */
    void setBinaryStream(int idx, ODBCXX_STREAM* s, int len);


    /** Sets a parameter value to NULL
     * @param idx The parameter index, starting at 1
     * @param sqlType The SQL type of the parameter
     * @see Types
     */
    void setNull(int idx, int sqlType);
  };


} // namespace odbc

#endif // __ODBCXX_PREPAREDSTATEMENT_H
