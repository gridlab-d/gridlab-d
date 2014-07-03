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

#ifndef __ODBCXX_RESULTSETMETADATA_H
#define __ODBCXX_RESULTSETMETADATA_H

#include <odbc++/setup.h>
#include <odbc++/types.h>
#include <odbc++/resultset.h>

namespace odbc {

  class ResultSet;
  class DriverInfo;

  /** Provides meta data about a result set */
  class ODBCXX_EXPORT ResultSetMetaData {
    friend class ResultSet;
    friend class Statement;
  private:
    ResultSet* resultSet_;

    int numCols_;
    std::vector<ODBCXX_STRING> colNames_;
    std::vector<int> colTypes_;
    std::vector<int> colPrecisions_;
    std::vector<int> colScales_;
#if ODBCVER >= 0x0300
    std::vector<int> colLengths_;
#endif

    //internal stuff
    bool needsGetData_;

    //yes, both constructor and destructor are meant to be private
    ResultSetMetaData(ResultSet* rs);
    ~ResultSetMetaData() {}

    //driver info
    const DriverInfo* _getDriverInfo() const {
      return resultSet_->_getDriverInfo();
    }

    //these fetch info about a column
    int _getNumericAttribute(unsigned int col, SQLUSMALLINT attr);
    ODBCXX_STRING _getStringAttribute(unsigned int col, SQLUSMALLINT attr, unsigned int maxlen =255);

    //this loads the above values
    void _fetchColumnInfo();

  public:
    /** Nullability constants */
    enum {
      columnNoNulls 		= SQL_NO_NULLS,
      columnNullable 		= SQL_NULLABLE,
      columnNullableUnknown 	= SQL_NULLABLE_UNKNOWN
    };

    /** Fetch the number of columns in this result set */
    int getColumnCount() const;

    /** Get the name of a column
     * @param column The column index, starting at 1
     */
    const ODBCXX_STRING& getColumnName(int column) const;

    /** Get the SQL type of a column
     * @param column The column index, starting at 1
     * @see Types
     */
    int getColumnType(int column) const;

    /** Get the precision of a column
     * @param column The column index, starting at 1
     */
    int getPrecision(int column) const;

    /** Get the scale of a column
     * @param column The column index, starting at 1
     */
    int getScale(int column) const;

    /** Get the display size of a column.
     * @param column The column index, starting at 1
     */
    int getColumnDisplaySize(int column);

    /** Get the catalog name for a column.
     * @param column The column index, starting at 1
     */
    ODBCXX_STRING getCatalogName(int column);

    /** Get the label (if any) for a column.
     * @param column The column index, starting at 1
     */
    ODBCXX_STRING getColumnLabel(int column);

    /** Get the name of a columns SQL type
     * @param column The column index, starting at 1
     */
    ODBCXX_STRING getColumnTypeName(int column);

    /** Get the schema name for a column 
     * @param column The column index, starting at 1
     */
    ODBCXX_STRING getSchemaName(int column);

    /** Get the table name for a column 
     * @param column The column index, starting at 1
     */
    ODBCXX_STRING getTableName(int column);

    /** Check if a column is autoincrementing 
     * @param column The column index, starting at 1
     */
    bool isAutoIncrement(int column);

    /** Check if a column is case sensitive
     * @param column The column index, starting at 1
     */
    bool isCaseSensitive(int column);
    
    /** Check if a column can be a currency (eg fixed precision)
     * @param column The column index, starting at 1
     */
    bool isCurrency(int column);

    /** Check if a column can be updated
     * @param column The column index, starting at 1
     */
    bool isDefinitelyWritable(int column);

    /** Check if a column can be set to NULL
     * @param column The column index, starting at 1
     */
    int isNullable(int column);

    /** Check if a column is read only
     * @param column The column index, starting at 1
     */
    bool isReadOnly(int column);

    /** Check if a column can be used in a where-clause
     * @param column The column index, starting at 1
     */
    bool isSearchable(int column);

    /** Check if a column is signed
     * @param column The column index, starting at 1
     */
    bool isSigned(int column);

    /** Check if a column is 'probably' writeable
     * @param column The column index, starting at 1
     */
    bool isWritable(int column);
  };

  

} // namespace odbc


#endif // __ODBCXX_RESULTSETMETADATA_H
