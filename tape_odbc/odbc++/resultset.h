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

#ifndef __ODBCXX_RESULTSET_H
#define __ODBCXX_RESULTSET_H

#include <odbc++/setup.h>
#include <odbc++/types.h>
#include <odbc++/errorhandler.h>
#include <odbc++/statement.h>


namespace odbc {

  class ResultSetMetaData;
  class Statement;
  class Rowset;

  /** A result set */
  class ODBCXX_EXPORT ResultSet : public ErrorHandler {
    friend class Statement;
    friend class ResultSetMetaData;
    
  private:
    Statement* statement_;
    SQLHSTMT hstmt_;
    bool ownStatement_;


    int currentFetchSize_;
    int newFetchSize_;

    Rowset* rowset_;
    SQLUSMALLINT* rowStatus_;
    SQLUINTEGER rowsInRowset_;

    //tells us if the columns are bound right now
    bool colsBound_;
    bool streamedColsBound_;

    //the position in the rowset last time we did a bind
    unsigned int bindPos_;

    //meta data - it's always there since we need info from it
    ResultSetMetaData* metaData_;

    int location_;
    
    bool lastWasNull_;

    int rowBeforeInsert_;
    int locBeforeInsert_;

    ResultSet(Statement* stmt,SQLHSTMT hstmt, bool ownStmt);
    
    //driver info
    const DriverInfo* _getDriverInfo() const {
      return statement_->_getDriverInfo();
    }

    //private utils
    void _applyFetchSize();
    //this makes sure there is a rowset 
    void _resetRowset();

    //this should be called before any call to SQLExtendedFetch
    void _prepareForFetch();
    //this performs a possibly scrolled fetch with fetchType to rownum
    void _doFetch(int fetchType, int rowNum);

    //this should be called after the position in the rowset changes
    SQLRETURN _applyPosition(int mode =SQL_POSITION);

    //these bind/unbind all non-streamed columns
    void _bindCols();
    void _unbindCols();

    //these bind/unbind all streamed columns
    void _bindStreamedCols();
    void _unbindStreamedCols();
    
    //this sends all needed data from streamed columns
    //to be called from insertRow and updateRow
    void _handleStreams(SQLRETURN r);
    

  public:
    /** Destructor */
    virtual ~ResultSet();
    
    //remember to update DatabaseMetaData when changing those values

    /** ResultSet concurrency constants.
     */
    enum {
      /** The ResultSet is read only */
      CONCUR_READ_ONLY,
      /** The ResultSet is updatable */
      CONCUR_UPDATABLE
    };


    /** ResultSet type constants 
     */
    enum {
      /** The result set only goes forward. */
      TYPE_FORWARD_ONLY,
      /** The result set is scrollable, but the data in it is not
       * affected by changes in the database.
       */
      TYPE_SCROLL_INSENSITIVE,
      /** The result set is scrollable and sensitive to database changes */
      TYPE_SCROLL_SENSITIVE
    };

    /** Moves the cursor to a specific row in this result set.
     * If row is negative, the actual row number is calculated from the
     * end of the result set. Calling <code>absolute(0)</code> is 
     * equivalent to calling <code>beforeFirst()</code>
     * @return true if the cursor is in the result set
     */
    bool absolute(int row);

    /** Moves the cursor inside the result set relative to the current row.
     * Negative values are allowed. This call is illegal if there is no
     * current row.
     * @return true if the cursor is in the result set
     */
    bool relative(int rows);

    /** Places the cursor after the last row in the result set */
    void afterLast();

    /** Places the cursor before the first row in the result set */
    void beforeFirst();

    /** Checks if the cursor is after the last row in the result set */
    bool isAfterLast();

    /** Checks if the cursor is before the first row in the result set */
    bool isBeforeFirst();

    /** Checks if the cursor is on the first row in the result set */
    bool isFirst();

    /** Checks if the cursor is on the last row in the result set.
     */
    bool isLast();

    /** Returns the current row number.
     * @return The current row number in the result set, or <code>0</code>
     * if it can't be determined.
     */
    int getRow();

    /** Moves to the next row in the result set 
     * @return true if the cursor is in the result set
     */
    bool next();

    /** Moves to the previous row in the result set 
     * @return true if the cursor is in the result set
     */
    bool previous();

    /** Moves to the first row in the result set 
     * @return true if the cursor is in the result set
     */
    bool first();

    /** Moves to the last row in the result set 
     * @return true if the cursor is in the result set
     */
    bool last();

    /** Moves the cursor to the 'insert row' of this result set.
     * @warning The only valid methods while on the insert row
     * are updateXXX(), insertRow() and moveToCurrentRow().
     * @see moveToCurrentRow()
     */
    void moveToInsertRow();
    
    /** Moves the cursor back to where it was before it was moved
     * to the insert row
     */
    void moveToCurrentRow();

    /** Refreshes the current row */
    void refreshRow();

    /** Deletes the current row */
    void deleteRow();

    /** Inserts the current row.
     * Only valid while on the insert row.
     * @see moveToInsertRow()
     */
    void insertRow();

    /** Updates the current row. */
    void updateRow();

    /** Cancels any updates done to the current row */
    void cancelRowUpdates();

    /** Returns meta data about this result set 
     * @see ResultSetMetaData
     */
    ResultSetMetaData* getMetaData() {
      return metaData_;
    }

    /** Find a column index by the column's name */
    int findColumn(const ODBCXX_STRING& colName);

    /** Checks if the current row is deleted */
    bool rowDeleted();

    /** Checks if the current row was inserted */
    bool rowInserted();

    /** Checks if the current row was updated */
    bool rowUpdated();

    /** Gets the type of this result set */
    int getType();

    /** Gets the concurrency of this result set */
    int getConcurrency();
    
    
    /** Gets this result set's current fetch size */
    int getFetchSize() {
      return newFetchSize_;
    }

    /** Sets this result set's fetch size (doesn't apply immediately) */
    void setFetchSize(int fetchSize);

    /** Gets the cursor name associated with this result set */
    ODBCXX_STRING getCursorName();

    /** Gets the Statement that created this result set */
    Statement* getStatement() {
      return statement_;
    }

    /** Gets a column's value as a double
     * @param idx The column index, starting at 1
     */
    double getDouble(int idx);

    /** Gets a column's value as a bool
     * @param idx The column index, starting at 1
     */
    bool getBoolean(int idx);

    /** Gets a column's value as a signed char
     * @param idx The column index, starting at 1
     */
    signed char getByte(int idx);

    /** Gets a column's value as a chunk of bytes.
     *
     * @param idx The column index, starting at 1
     */
    ODBCXX_BYTES getBytes(int idx);

    /** Gets a column's value as a Date
     * @param idx The column index, starting at 1
     */
    Date getDate(int idx);

    /** Gets a column's value as a float
     * @param idx The column index, starting at 1
     */
    float getFloat(int idx);

    /** Gets a column's value as an int
     * @param idx The column index, starting at 1
     */
    int getInt(int idx);

    /** Gets a column's value as a Long
     * @param idx The column index, starting at 1
     */
    Long getLong(int idx);

    /** Gets a column's value as a short
     * @param idx The column index, starting at 1
     */
    short getShort(int idx);

    /** Gets a column's value as a string
     * @param idx The column index, starting at 1
     */
    ODBCXX_STRING getString(int idx);

    /** Gets a column's value as a Time
     * @param idx The column index, starting at 1
     */
    Time getTime(int idx);

    /** Gets a column's value as a Timestamp
     * @param idx The column index, starting at 1
     */
    Timestamp getTimestamp(int idx);

    /** Gets a column's value as a double
     * @param colName The name of the column
     */
    double getDouble(const ODBCXX_STRING& colName);
    
    /** Gets a column's value as a bool
     * @param colName The name of the column
     */
    bool getBoolean(const ODBCXX_STRING& colName);

    /** Gets a column's value as a signed char
     * @param colName The name of the column
     */
    signed char getByte(const ODBCXX_STRING& colName);


    /** Gets a column's value as a chunk of bytes.
     * @param colName The name of the column
     */
    ODBCXX_BYTES getBytes(const ODBCXX_STRING& colName);

    /** Gets a column's value as a Date
     * @param colName The name of the column
     */
    Date getDate(const ODBCXX_STRING& colName);

    /** Gets a column's value as a float
     * @param colName The name of the column
     */
    float getFloat(const ODBCXX_STRING& colName);

    /** Gets a column's value as an int
     * @param colName The name of the column
     */
    int getInt(const ODBCXX_STRING& colName);

    /** Gets a column's value as a Long
     * @param colName The name of the column
     */
    Long getLong(const ODBCXX_STRING& colName);

    /** Gets a column's value as a short
     * @param colName The name of the column
     */
    short getShort(const ODBCXX_STRING& colName);

    /** Gets a column's value as a string
     * @param colName The name of the column
     */
    ODBCXX_STRING getString(const ODBCXX_STRING& colName);

    /** Gets a column's value as a Time
     * @param colName The name of the column
     */
    Time getTime(const ODBCXX_STRING& colName);

    /** Gets a column's value as a Timestamp
     * @param colName The name of the column
     */
    Timestamp getTimestamp(const ODBCXX_STRING& colName);


    /** Fetches a column's value as a stream.
     * Note that the stream is owned by the result set
     * and should in no case be deleted by the caller.
     * Also, the returned stream is only valid while the 
     * cursor remains on this position.
     * @param idx The column index, starting at 1
     */
    ODBCXX_STREAM* getAsciiStream(int idx);

    /** Fetches a column's value as a stream.
     * Note that the stream is owned by the result set
     * and should in no case be deleted by the caller.
     * Also, the returned stream is only valid while the 
     * cursor remains on this position.
     * @param colName The column name
     */
    ODBCXX_STREAM* getAsciiStream(const ODBCXX_STRING& colName);

    /** Fetches a column's value as a stream.
     * Note that the stream is owned by the result set
     * and should in no case be deleted by the caller.
     * Also, the returned stream is only valid while the 
     * cursor remains on this position.
     * @param idx The column index, starting at 1
     */
    ODBCXX_STREAM* getBinaryStream(int idx);

    /** Fetches a column's value as a stream.
     * Note that the stream is owned by the result set
     * and should in no case be deleted by the caller.
     * Also, the returned stream is only valid while the 
     * cursor remains on this position.
     * @param colName The column name
     */
    ODBCXX_STREAM* getBinaryStream(const ODBCXX_STRING& colName);
    
    /** Checks if the last fetched column value was NULL.
     * Note that if this is true, the returned value was undefined.
     */
    bool wasNull() {
      return lastWasNull_;
    }

    /** Sets the value of a column to a double
     * @param idx The column index, starting at 1
     * @param val The value to set
     */
    void updateDouble(int idx, double val);

    /** Sets the value of a column to a bool
     * @param idx The column index, starting at 1
     * @param val The value to set
     */
    void updateBoolean(int idx, bool val);

    /** Sets the value of a column to a signed char
     * @param idx The column index, starting at 1
     * @param val The value to set
     */
    void updateByte(int idx, signed char val);


    /** Sets the value of a column to a chunk of bytes
     * @param idx The column index, starting at 1
     * @param val The value to set
     */
    void updateBytes(int idx, const ODBCXX_BYTES& val);

    /** Sets the value of a column to a Date
     * @param idx The column index, starting at 1
     * @param val The value to set
     */
    void updateDate(int idx, const Date& val);

    /** Sets the value of a column to a float
     * @param idx The column index, starting at 1
     * @param val The value to set
     */
    void updateFloat(int idx, float val);

    /** Sets the value of a column to an int
     * @param idx The column index, starting at 1
     * @param val The value to set
     */
    void updateInt(int idx, int val);

    /** Sets the value of a column to a Long
     * @param idx The column index, starting at 1
     * @param val The value to set
     */
    void updateLong(int idx, Long val);

    /** Sets the value of a column to a short
     * @param idx The column index, starting at 1
     * @param val The value to set
     */
    void updateShort(int idx, short val);

    /** Sets the value of a column to a string
     * @param idx The column index, starting at 1
     * @param val The value to set
     */
    void updateString(int idx, const ODBCXX_STRING& val);

    /** Sets the value of a column to a Time
     * @param idx The column index, starting at 1
     * @param val The value to set
     */
    void updateTime(int idx, const Time& val);

    /** Sets the value of a column to a Timestamp
     * @param idx The column index, starting at 1
     * @param val The value to set
     */
    void updateTimestamp(int idx, const Timestamp& val);

    /** Sets the value of a column to NULL
     * @param idx The column index, starting at 1
     */
    void updateNull(int idx);


    /** Sets the value of a column to a double 
     * @param colName The name of the column
     * @param val The value to set
     */
    void updateDouble(const ODBCXX_STRING& colName, double val);

    /** Sets the value of a column to a bool
     * @param colName The name of the column
     * @param val The value to set
     */
    void updateBoolean(const ODBCXX_STRING& colName, bool val);

    /** Sets the value of a column to a signed char
     * @param colName The name of the column
     * @param val The value to set
     */
    void updateByte(const ODBCXX_STRING& colName, signed char val);

    /** Sets the value of a column to a chunk of bytes
     * @param colName The name of the column
     * @param val The value to set
     */
    void updateBytes(const ODBCXX_STRING& colName, 
		     const ODBCXX_BYTES& val);


    /** Sets the value of a column to a Date
     * @param colName The name of the column
     * @param val The value to set
     */
    void updateDate(const ODBCXX_STRING& colName, const Date& val);

    /** Sets the value of a column to a float
     * @param colName The name of the column
     * @param val The value to set
     */
    void updateFloat(const ODBCXX_STRING& colName, float val);

    /** Sets the value of a column to an int
     * @param colName The name of the column
     * @param val The value to set
     */
    void updateInt(const ODBCXX_STRING& colName, int val);

    /** Sets the value of a column to a Long
     * @param colName The name of the column
     * @param val The value to set
     */
    void updateLong(const ODBCXX_STRING& colName, Long val);

    /** Sets the value of a column to a short
     * @param colName The name of the column
     * @param val The value to set
     */
    void updateShort(const ODBCXX_STRING& colName, short val);

    /** Sets the value of a column to a string
     * @param colName The name of the column
     * @param val The value to set
     */
    void updateString(const ODBCXX_STRING& colName, const ODBCXX_STRING& val);

    /** Sets the value of a column to a Time
     * @param colName The name of the column
     * @param val The value to set
     */
    void updateTime(const ODBCXX_STRING& colName, const Time& val);

    /** Sets the value of a column to a Timestamp
     * @param colName The name of the column
     * @param val The value to set
     */
    void updateTimestamp(const ODBCXX_STRING& colName, const Timestamp& val);

    /** Sets the value of a column to a stream
     * @param idx The column index, starting at 1
     * @param s The stream to assign
     * @param len The number of bytes in the stream
     */
    void updateAsciiStream(int idx, ODBCXX_STREAM* s, int len);

    /** Sets the value of a column to the contens of a stream
     * @param colName The column name
     * @param s The stream to assign
     * @param len The number of bytes in the stream
     */
    void updateAsciiStream(const ODBCXX_STRING& colName, ODBCXX_STREAM* s, int len);


    /** Sets the value of a column to the contens of a stream
     * @param idx The column index, starting at 1
     * @param s The stream to assign
     * @param len The number of bytes in the stream
     */
    void updateBinaryStream(int idx, ODBCXX_STREAM* s, int len);

    /** Sets the value of a column to the contens of a stream
     * @param colName The column name
     * @param s The stream to assign
     * @param len The number of bytes in the stream
     */
    void updateBinaryStream(const ODBCXX_STRING& colName, ODBCXX_STREAM* s, int len);

    /** Sets the value of a column to NULL
     * @param colName The column name
     */
    void updateNull(const ODBCXX_STRING& colName);
  };

  

} // namespace odbc


#endif // __ODBCXX_RESULTSET_H
