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

#ifndef __ODBCXX_DATABASEMETADATA_H
#define __ODBCXX_DATABASEMETADATA_H

#include <odbc++/setup.h>
#include <odbc++/types.h>
#include <odbc++/connection.h>

namespace odbc {

  class ResultSet;
  class DriverInfo;

  /** Provides several tons of information about a data source.
   *
   * @warning The column names in ResultSets returned by methods of
   * DatabaseMetaData can differ depending on what ODBC version
   * the current driver supports. To avoid problems, columns should
   * be referenced by number, and not by name. Also note that
   * ODBC version 2 drivers do not return some of the specified
   * columns.
   */
  class ODBCXX_EXPORT DatabaseMetaData {
    friend class Connection;
    friend class DriverInfo;

  private:
    Connection* connection_;

    DatabaseMetaData(Connection* c);
    ~DatabaseMetaData();

    const DriverInfo* _getDriverInfo() const {
      return connection_->_getDriverInfo();
    }

    SQLUSMALLINT _getNumeric16(int what);
    SQLUINTEGER _getNumeric32(int what);

    ODBCXX_STRING _getStringInfo(int what);
    bool _ownXXXAreVisible(int type, int what);

#if ODBCVER >= 0x0300
    // returns all CA1 or-ed together
    SQLUINTEGER _getAllCursorAttributes1();
#endif

  public:

    /** Returns the Connection this came from */
    Connection* getConnection() {
      return connection_;
    }

    /** Constants for the ResultSet returned by getBestRowIdentifier */
    enum {
      bestRowTemporary		= SQL_SCOPE_CURROW,
      bestRowTransaction	= SQL_SCOPE_TRANSACTION,
      bestRowSession		= SQL_SCOPE_SESSION
    };

    /** Constants for the ResultSet returned by getBestRowIdentifier */
    enum {
      bestRowUnknown		= SQL_PC_UNKNOWN,
      bestRowPseudo		= SQL_PC_PSEUDO,
      bestRowNotPseudo		= SQL_PC_NOT_PSEUDO
    };


    /** Version column constants for getVersionColumns()
     * @see #getVersionColumns()
     */
    enum {
      versionColumnNotPseudo	= SQL_PC_NOT_PSEUDO,
      versionColumnPseudo	= SQL_PC_PSEUDO,
      versionColumnUnknown	= SQL_PC_UNKNOWN
    };


    /** Nullability constants for the resultset returned by getTypes()
     * @see getTypes()
     */
    enum {
      typeNoNulls		= SQL_NO_NULLS,
      typeNullable		= SQL_NULLABLE,
      typeNullableUnknown	= SQL_NULLABLE_UNKNOWN
    };

    /** Nullability constants for the resultset returned by
     * getColumns().
     * @see getColumns()
     */
    enum {
      columnNoNulls		= SQL_NO_NULLS,
      columnNullable		= SQL_NULLABLE,
      columnNullableUnknown	= SQL_NULLABLE_UNKNOWN
    };

    /** Searchability constants */
    enum {
      /** Column is unsearchable */
      typePredNone 		= SQL_UNSEARCHABLE,
      /** Column can only be used in a LIKE clause */
      typePredChar		= SQL_LIKE_ONLY,
      /** Column can be used in searches, except in LIKE */
      typePredBasic		= SQL_ALL_EXCEPT_LIKE,
      /** Column is searchable */
      typeSearchable		= SQL_SEARCHABLE
    };


    /** Imported key UPDATE_RULE and DELETE_RULE constants.
     * @see getImportedKeys()
     */
#if ODBCVER >= 0x0300
    enum {
      importedKeyCascade	= SQL_CASCADE,
      importedKeySetNull	= SQL_SET_NULL,
      importedKeySetDefault	= SQL_SET_DEFAULT,
      importedKeyNoAction	= SQL_NO_ACTION,
      importedKeyRestrict	= SQL_RESTRICT
    };
#else
    // workaround mode on
    enum {
      importedKeyCascade	= SQL_CASCADE,
      importedKeySetNull	= SQL_SET_NULL,
      importedKeyRestrict	= SQL_RESTRICT,
      importedKeyNoAction	= SQL_RESTRICT,
      importedKeySetDefault
    };

#endif

#if ODBCVER >= 0x0300
#if !defined(SQL_NOT_DEFERRABLE)
# warning "Your sqlext.h is missing SQL_NOT_DEFERRABLE, consider upgrading"
# define SQL_NOT_DEFERRABLE 7
#endif
    /** Imported key DEFERRABILITY constants */
    enum {
      importedKeyInitiallyDeferred	= SQL_INITIALLY_DEFERRED,
      importedKeyInitiallyImmediate	= SQL_INITIALLY_IMMEDIATE,
      importedKeyNotDeferrable		= SQL_NOT_DEFERRABLE
    };
#endif

    /** Index type constants */
    enum {
      tableIndexClustered	= SQL_INDEX_CLUSTERED,
      tableIndexHashed		= SQL_INDEX_HASHED,
      tableIndexOther		= SQL_INDEX_OTHER,
      tableIndexStatistic	= SQL_TABLE_STAT
    };

    /** Procedure column type constants for getProcedureColumns()
     * @see #getProcedureColumns()
     */
    enum {
      procedureColumnIn		= SQL_PARAM_INPUT,
      procedureColumnInOut	= SQL_PARAM_INPUT_OUTPUT,
      procedureColumnOut	= SQL_PARAM_OUTPUT,
      procedureColumnResult	= SQL_RESULT_COL,
      procedureColumnReturn	= SQL_RETURN_VALUE,
      procedureColumnUnknown	= SQL_PARAM_TYPE_UNKNOWN
    };

    /** Procedure column nullability constants for getProcedureColumns()
     * @see #getProcedureColumns()
     */
    enum {
      procedureNoNulls		= SQL_NO_NULLS,
      procedureNullable		= SQL_NULLABLE,
      procedureNullableUnknown	= SQL_NULLABLE_UNKNOWN
    };

    /** Procedure type constants for PROCEDURE_TYPE in getProcedures()
     * @see #getProcedures()
     */
    enum {
      procedureReturnsResult	= SQL_PT_FUNCTION,
      procedureNoResult		= SQL_PT_PROCEDURE,
      procedureResultUnknown	= SQL_PT_UNKNOWN
    };



    /** Returns the name of the database product.
     */
    ODBCXX_STRING getDatabaseProductName();

    /** Returns the version of the database product as a string.
     */
    ODBCXX_STRING getDatabaseProductVersion();

    /** Returns the name of the ODBC driver used.
     */
    ODBCXX_STRING getDriverName();

    /** Returns the version of the ODBC driver used.
     */
    ODBCXX_STRING getDriverVersion();

    /** Returns the major ODBC version of the driver used. */
    int getDriverMajorVersion();

    /** Returns the minor ODBC version of the driver used. */
    int getDriverMinorVersion();

    /** Returns the string that can be used to quote identifiers.
     * If the data source doesn't support it, returns an empty string.
     */
    ODBCXX_STRING getIdentifierQuoteString();

    /** Returns the term for catalog used by the data source.
     * Can be for example "directory" or "database".
     */
    ODBCXX_STRING getCatalogTerm();

    /** Returns the term for schema used by the data source,
     * for example "owner" or just "schema".
     */
    ODBCXX_STRING getSchemaTerm();

    /** Returns the term for table used by the data source.
     */
    ODBCXX_STRING getTableTerm();

    /** Returns the term the data source uses for a procedure,
     * for example "stored procedure".
     */
    ODBCXX_STRING getProcedureTerm();

    /** Returns the user name of the connection.
     */
    ODBCXX_STRING getUserName();

    /** Returns the string used to separate a catalog
     * in a fully qualified identifier.
     *
     * For example Oracle would return a "@", while
     * mysql would say ".".
     */
    ODBCXX_STRING getCatalogSeparator();

    /** Returns true if the catalog is positioned at the
     * beginning of a fully qualified identifier.
     *
     * For example mysql would say true, while oracle would say false.
     */
    bool isCatalogAtStart();

    /** Returns a comma-separated list of all non-ODBC keywords specific
     * to this data source.
     */
    ODBCXX_STRING getSQLKeywords();


    /** Returns true if the data source supports transactions.
     */
    bool supportsTransactions();

    /** Returns the default transaction isolation level
     * @see Connection
     */
    int getDefaultTransactionIsolation();

    /** Returns true if the data source supports the specified transaction
     * isolation level.
     * @param lev The isolation level to check for
     */
    bool supportsTransactionIsolationLevel(int lev);

    /** Checks if the data source supports both DML and DDL in transactions
     *
     * @return <code>true</code> if the data source supports both data manipulation
     * (eg. <code>UPDATE</code>, <code>INSERT</code>) and data definition
     * (eg. <code>CREATE TABLE</code>) within a transaction.
     *
     *
     * If this method returns <code>true</code>,
     * supportsDataManipulationTransactionsOnly(),
     * dataDefinitionCausesTransactionCommit() and
     * dataDefinitionIgnoredInTransactions() all return <code>false</code>.
     */
    bool supportsDataDefinitionAndDataManipulationTransactions();

    /** Checks if the data source only supports DML in transactions
     *
     * @return <code>true</code> if the data source only supports data manipulation
     * (eg. <code>UPDATE</code>, <code>INSERT</code>) within a transaction.
     *
     * Attempts to use data definition
     * (eg. <code>CREATE TABLE</code>) in a transaction will trigger an
     * error.
     *
     * If this method returns <code>true</code>,
     * supportsDataDefinitionAndDataManipulationTransactions(),
     * dataDefinitionCausesTransactionCommit() and
     * dataDefinitionIgnoredInTransactions() all return <code>false</code>.
     */
    bool supportsDataManipulationTransactionsOnly();

    /** Checks if DDL in a transaction will cause the transaction to be committed
     *
     * @return true if the data source only supports data manipulation
     * (eg. <code>UPDATE</code>, <code>INSERT</code>) within a transaction
     * and any data definition (eg. <code>CREATE TABLE</code>) will cause
     * the transaction to be committed.
     *
     * If this method returns <code>true</code>,
     * supportsDataDefinitionAndDataManipulationTransactions(),
     * supportsDataManipulationTransactionsOnly() and
     * dataDefinitionIgnoredInTransactions() all return <code>false</code>.
     */
    bool dataDefinitionCausesTransactionCommit();

    /** Checks if DDL in a transaction is ignored
     *
     * @return true if the data source only supports data manipulation
     * (eg. <code>UPDATE</code>, <code>INSERT</code>) within a transaction
     * and any data definition (eg. <code>CREATE TABLE</code>) will be
     * ignored.
     *
     * If this method returns <code>true</code>,
     * supportsDataDefinitionAndDataManipulationTransactions(),
     * supportsDataManipulationTransactionsOnly() and
     * dataDefinitionCausesTransactionCommit() all return <code>false</code>.
     */
    bool dataDefinitionIgnoredInTransactions();


    /**
     */
    bool supportsTableCorrelationNames();

    /**
     */
    bool supportsDifferentTableCorrelationNames();

    /**
     */
    bool supportsOrderByUnrelated();

    /**
     */
    bool supportsExpressionsInOrderBy();

    /** Returns true if the data source and the driver can handle
     * open cursors (eg. ResultSets) across a commit, or false if
     * they are invalidated.
     */
    bool supportsOpenCursorsAcrossCommit();


    /** Returns true if the data source and the driver can handle
     * open cursors (eg. ResultSets) across a rollback, or false if
     * they are invalidated.
     */
    bool supportsOpenCursorsAcrossRollback();


    /** Returns true if the data source and the driver can handle
     * open statements across a commit, or false if
     * they are invalidated.
     */
    bool supportsOpenStatementsAcrossCommit();

    /** Returns true if the data source and the driver can handle
     * open statements across a rollback, or false if
     * they are invalidated.
     */
    bool supportsOpenStatementsAcrossRollback();

    /** Returns true if the data source supports the given
     * result set type.
     * @param type The type to check for
     * @see ResultSet
     */
    bool supportsResultSetType(int type);

    /** Returns true if the data source supports the given
     * result set concurrency for the given result set type.
     * @param type The type to check for.
     * @param concurrency The concurrency level to check for.
     * @see ResultSet
     */
    bool supportsResultSetConcurrency(int type, int concurrency);


    /** Returns true if catalogs are supported in DML */
    bool supportsCatalogsInDataManipulation();

    /** Returns true if catalogs are supported in procedure call statements */
    bool supportsCatalogsInProcedureCalls();

    /** Returns true if catalogs are supported in CREATE/ALTER TABLE statements */
    bool supportsCatalogsInTableDefinitions();

    /** Returns true if catalogs are supported in index definitions */
    bool supportsCatalogsInIndexDefinitions();

    /** Returns true if catalogs are supported in privilege definition statements */
    bool supportsCatalogsInPrivilegeDefinitions();


    /** Returns true if schemas are supported in DML */
    bool supportsSchemasInDataManipulation();

    /** Returns true if schemas are supported in procedure call statements */
    bool supportsSchemasInProcedureCalls();

    /** Returns true if schemas are supported in CREATE/ALTER TABLE statements */
    bool supportsSchemasInTableDefinitions();

    /** Returns true if schemas are supported in index definitions */
    bool supportsSchemasInIndexDefinitions();

    /** Returns true if schemas are supported in privilege definition statements */
    bool supportsSchemasInPrivilegeDefinitions();


    /** Returns true if NULL plus a non-NULL value yields NULL.
     */
    bool nullPlusNonNullIsNull();

    /** Returns true if the data source supports column aliasing, for example
     * SELECT COLUMN1 [AS] C1 FROM TABLE
     */
    bool supportsColumnAliasing();

    /** Returns true if the CONVERT function is supported by the data source.
     */
    bool supportsConvert();

    /** Returns true if CONVERT between fromType and toType is supported.
     */
    bool supportsConvert(int fromType, int toType);

    /** Returns true if ALTER TABLE with drop column is supported.
     */
    bool supportsAlterTableWithDropColumn();

    /** Returns true if ALTER TABLE with add column is supported.
     */
    bool supportsAlterTableWithAddColumn();

    /** Returns the extra characters beyond A-Z, a-z, 0-9 and _ that can
     * be used in unquoted identifier names.
     */
    ODBCXX_STRING getExtraNameCharacters();

    /** Returns the string that can be used to escape wildcard characters.
     */
    ODBCXX_STRING getSearchStringEscape();

    /** Returns a comma-separated list of all time and date functions supported.
     */
    ODBCXX_STRING getTimeDateFunctions();

    /** Returns a comma-separated list of all system functions supported.
     */
    ODBCXX_STRING getSystemFunctions();

    /** Returns a comma-separated list of all string functions supported.
     */
    ODBCXX_STRING getStringFunctions();

    /** Returns a comma-separated list of all numeric functions supported.
     */
    ODBCXX_STRING getNumericFunctions();

    /** Returns true if the escape character is supported in LIKE clauses.
     */
    bool supportsLikeEscapeClause();

    /** Returns true if a query can return multiple ResultSets.
     */
    bool supportsMultipleResultSets();

    /** Returns true if multiple transactions can be open at once
     * on different connections.
     */
    bool supportsMultipleTransactions();

    /** Returns true if columns can be defined as non-nullable.
     */
    bool supportsNonNullableColumns();

    /** Returns true if the data source supports ODBC minimum SQL grammar.
     */
    bool supportsMinimumSQLGrammar();

    /** Returns true if the data source supports the core ODBC SQL grammar.
     */
    bool supportsCoreSQLGrammar();

    /** Returns true if the data source supports the ODBC extended SQL grammar.
     */
    bool supportsExtendedSQLGrammar();


    /** Returns true if the data source supports the ANSI92 entry level
     * SQL grammar.
     */
    bool supportsANSI92EntryLevelSQL();

    /** Returns true if the data source supports the ANSI92 intermediate level
     * SQL grammar.
     */
    bool supportsANSI92IntermediateSQL();

    /** Returns true if the data source supports the full ANSI92 SQL grammar.
     */
    bool supportsANSI92FullSQL();

    /** Checks if the data source supports positioned delete
     *
     * @return <code>true</code> if (<code>"DELETE WHERE CURRENT OF ..."</code>)
     * is supported
     */
    bool supportsPositionedDelete();

    /** Checks if the data source supports positioned update
     *
     * @return <code>true</code> if (<code>"UPDATE ... WHERE CURRENT OF ..."</code>)
     * is supported
     */
    bool supportsPositionedUpdate();

    /** Checks if the data source supports
     *
     * @return <code>true</code> if (<code>"SELECT ... FOR UPDATE"</code>)
     * is supported
     */
    bool supportsSelectForUpdate();


    /** Returns true if the data source supports the SQL Integrity
     * Enhancement Facility.
     */
    bool supportsIntegrityEnhancementFacility();

    /** Whether the data source supports batch updates
     */
    bool supportsBatchUpdates();

    /** Returns true if the data source supports subqueries in comparisons
     */
    bool supportsSubqueriesInComparisons();

    /** Returns true if the data source supports subqueries in
     * <code>"EXISTS"</code> expressions.
     */
    bool supportsSubqueriesInExists();

    /** Returns true if the data source supports subqueries in
     * <code>"IN"</code> expressions.
     */
    bool supportsSubqueriesInIns();

    /** Returns true if the data source supports subqueries in
     * quantified expressions.
     */
    bool supportsSubqueriesInQuantifieds();


    /** Returns true if the data source supports correlated subqueries
     */
    bool supportsCorrelatedSubqueries();

    /** Returns true if updated rows are available with their new
     * values in the ResultSet.
     * @param type The type of ResultSet of interest
     */
    bool ownUpdatesAreVisible(int type);

    /** Returns true if deleted rows dissapear from a ResultSet
     * @param type The type of ResultSet of interest
     */
    bool ownDeletesAreVisible(int type);

    /** Returns true if inserted rows become available in a ResultSet
     * @param type The type of ResultSet of interest
     */
    bool ownInsertsAreVisible(int type);

    /** Returns true if rows updated by others are visible
     * with their new values.
     * @param type The type of ResultSet of interest
     */
    bool othersUpdatesAreVisible(int type);

    /** Returns true if rows deleted by others disapear
     * from a ResultSet.
     * @param type The type of ResultSet of interest
     */
    bool othersDeletesAreVisible(int type);

    /** Returns true if rows inserted by others become available in
     * a ResultSet.
     * @param type The type of ResultSet of interest
     */
    bool othersInsertsAreVisible(int type);

    /** Returns true if a deleted row can be detected with
     * ResultSet::rowDeleted().
     * @param type The type of ResultSet of interest
     */
    bool deletesAreDetected(int type);

    /** Returns true if an inserted row can be detected with
     * ResultSet::rowInserted().
     * @param type The type of ResultSet of interest
     */
    bool insertsAreDetected(int type);

    /** Returns true if ResultSet::rowUpdated can determine whether
     * a row has been updated.
     * @param type The type of ResultSet of interest
     */
    bool updatesAreDetected(int type);


    /** Returns the max number of hex characters allowed in an inline binary
     * literal.
     */
    int getMaxBinaryLiteralLength();

    /** Returns the maximum length of an inline character string.
     */
    int getMaxCharLiteralLength();

    /** Returns the maximum length of a column name.
     */
    int getMaxColumnNameLength();

    /** Returns the maximum number of columns this data source can have in
     * a GROUP BY clause.
     */
    int getMaxColumnsInGroupBy();

    /** Returns the maximum number of columns allowed in an index.
     */
    int getMaxColumnsInIndex();

    /** Returns the maximum number of columns this data source can have in
     * an ORDER BY clause.
     */
    int getMaxColumnsInOrderBy();

    /** Returns the maximum number of columns this data source can SELECT.
     */
    int getMaxColumnsInSelect();

    /** Returns the maximum number of columns a table can consist of.
     */
    int getMaxColumnsInTable();

    /** Returns the maximum length of a cursor name.
     */
    int getMaxCursorNameLength();

    /** Returns the maximum length of an index in bytes.
     */
    int getMaxIndexLength();

    /** Returns the maximum length of a schema name.
     */
    int getMaxSchemaNameLength();

    /** Returns the maximum length of a procedure name.
     */
    int getMaxProcedureNameLength();

    /** Returns the maximum length of a catalog name.
     */
    int getMaxCatalogNameLength();

    /** Returns the maximum size of a row in bytes.
     */
    int getMaxRowSize();

    /** Returns true if the value returned by getMaxRowSize() includes
     * BLOBs.
     */
    bool doesMaxRowSizeIncludeBlobs();


    /** Returns the maximum length of a statement (query).
     */
    int getMaxStatementLength();

    /** Returns the maximum length of a table name.
     */
    int getMaxTableNameLength();

    /** Returns the maximum number of tables that can be joined at once.
     * @return zero if unknown.
     */
    int getMaxTablesInSelect();

    /** Returns the maximum length of a username.
     */
    int getMaxUserNameLength();

    /** Returns the maximum number of connections we can have open
     * to this data source.
     */
    int getMaxConnections();

    /** Returns the maximim number of statements that can be open
     * on this connection.
     */
    int getMaxStatements();


    /** Returns true if the data source supports case sensitive
     * mixed case identifiers.
     */
    bool supportsMixedCaseIdentifiers();

    /** Returns true if the data source supports case sensitive
     * mixed case quoted identifiers.
     */
    bool supportsMixedCaseQuotedIdentifiers();

    /** Returns true if the data source supports some form of
     * stored procedures.
     */
    bool supportsStoredProcedures();


    /** Returns true if the data source supports the
     * GROUP BY clause.
     */
    bool supportsGroupBy();

    /** Returns true if the columns in a GROUP BY clause are independent
     * of the selected ones.
     */
    bool supportsGroupByUnrelated();

    /** Returns true if the columns in a GROUP BY don't have to be
     * selected.
     */
    bool supportsGroupByBeyondSelect();


    /** Returns true if the data source supports UNION joins.
     */
    bool supportsUnion();

    /** Returns true if the data source supports UNION ALL joins.
     */
    bool supportsUnionAll();

    /** Returns true if the data source supports some form of
     * outer joins.
     */
    bool supportsOuterJoins();

    /** Returns true if the data source fully supports outer joins.
     */
    bool supportsFullOuterJoins();

    /** Returns true if the data source only supports certain types of
     * outer joins.
     */
    bool supportsLimitedOuterJoins();

    /** Returns true if the data source uses a file for each table */
    bool usesLocalFilePerTable();

    /** Returns true if the data source uses local files */
    bool usesLocalFiles();

    /** Returns true if NULL values are sorted first, regardless of
     * the sort order.
     */
    bool nullsAreSortedAtStart();

    /** Returns true if NULL values are sorted last, regardless of
     * the sort order.
     */
    bool nullsAreSortedAtEnd();

    /** Returns true if NULL values are sorted high.
     */
    bool nullsAreSortedHigh();

    /** Returns true if NULL values are sorted low.
     */
    bool nullsAreSortedLow();

    /** Returns true if all procedures returned by getProcedures() are callable
     * by the current user.
     */
    bool allProceduresAreCallable();

    /** Returns true if all tables returned by getTables() are selectable by
     * the current user.
     */
    bool allTablesAreSelectable();

    /** Returns true if the data source or the current connection is in
     * read-only mode.
     */
    bool isReadOnly();

    /** Returns true if the data source treats identifiers as case
     * insensitive and stores them in lower case.
     */
    bool storesLowerCaseIdentifiers();

    /** Returns true if the data source treats quoted identifiers as case
     * insensitive and stores them in lower case.
     */
    bool storesLowerCaseQuotedIdentifiers();

    /** Returns true if the data source treats identifiers as case
     * insensitive and stores them in mixed case.
     */
    bool storesMixedCaseIdentifiers();

    /** Returns true if the data source treats quoted identifiers as case
     * insensitive and stores them in mixed case.
     */
    bool storesMixedCaseQuotedIdentifiers();

    /** Returns true if the data source treats identifiers as case
     * insensitive and stores them in upper case.
     */
    bool storesUpperCaseIdentifiers();

    /** Returns true if the data source treats quoted identifiers as case
     * insensitive and stores them in upper case.
     */
    bool storesUpperCaseQuotedIdentifiers();


    /** Fetches a list of data types supported by this data source.
     *
     * The returned ResultSet is ordered by <code>DATA_TYPE</code> and then
     * by how closely the type maps to the corresponding ODBC SQL type.
     * It contains the following columns:
     * <ol>
     * <li><b>TYPE_NAME</b> - string - native type name
     * <li><b>DATA_TYPE</b> - short - SQL data type from Types
     * <li><b>COLUMN_SIZE</b> - int - maximum precision
     * <li><b>LITERAL_PREFIX</b> - string - prefix used to quote a literal.
     *     Can be <code>NULL</code>.
     * <li><b>LITERAL_SUFFIX</b> - string - suffix used to quote a literal.
     *     Can be <code>NULL</code>.
     * <li><b>CREATE_PARAMS</b> - string - comma separated possible list of
     *     parameters to creating a column of this type
     * <li><b>NULLABLE</b> - short - whether this type can contain <code>NULL</code>s:
     *     <ul>
     *     <li><code>typeNoNulls</code> - no
     *     <li><code>typeNullable</code> - yes, can be nullable
     *     <li><code>typeNullableUnknown</code> - nobody knows
     *     </ul>
     * <li><b>CASE_SENSITIVE</b> - bool - whether this type is case sensitive
     * <li><b>SEARCHABLE</b> - bool - whether this type can be searched, eg
     *     used in <code>WHERE</code>-clauses:
     *     <ul>
     *     <li><code>typePredNone</code> - no
     *     <li><code>typePredChar</code> - yes, but only with a <code>LIKE</code>
     *         predicate
     *     <li><code>typePredBasic</code> - yes, except in a <code>LIKE</code>
     *         predicate
     *     <li><code>typeSearchable</code> - yes
     *     </ul>
     * <li><b>UNSIGNED_ATTRIBUTE</b> - bool - <code>true</code> if this type is unsigned
     * <li><b>FIXED_PREC_SCALE</b> - bool - whether this type has predefined fixed
     *     precision and scale (eg is useful for money)
     * <li><b>AUTO_UNIQUE_VALUE</b> - bool - whether this type can be used for an
     *     autoincrementing value. <code>NULL</code> if not applicable.
     * <li><b>LOCAL_TYPE_NAME</b> - string - localized native type name. Can be
     *     <code>NULL</code>.
     * <li><b>MINIMUM_SCALE</b> - short - minimum supported scale, if applicable
     * <li><b>MAXIMUM_SCALE</b> - short - maximum supported scale, if applicable
     * <li><b>SQL_DATA_TYPE</b> - short - unused
     * <li><b>SQL_DATETIME_SUB</b> - short - unused
     * <li><b>NUM_PREC_RADIX</b> - int - radix, if applicable
     * </ol>
     */
    ResultSet* getTypeInfo();

    /** Fetches the available columns in a catalog.
     *
     * The returned ResultSet has the following columns:
     * <ol>
     * <li><b>TABLE_CAT</b> - string - table catalog (can be NULL)
     * <li><b>TABLE_SCHEM</b> - string - table schema (can be NULL)
     * <li><b>TABLE_NAME</b> - string - table name
     * <li><b>COLUMN_NAME</b> - string - column name
     * <li><b>COLUMN_TYPE</b> - short - see Types
     * <li><b>TYPE_NAME</b> - string - the name of the type. Data source
     * dependent.
     * <li><b>COLUMN_SIZE</b> - int - column size. For character and date
     * types, this is the maximum number of characters. For numeric types
     * this is the precision.
     * <li><b>BUFFER_LENGTH</b> - not used
     * <li><b>DECIMAL_DIGITS</b> - int - the number of fractional digits.
     * <li><b>NUM_PREC_RADIX</b> - int - radix
     * <li><b>NULLABLE</b> - int - whether the column allows NULLs
     *    <ul>
     *    <li><code>columnNoNulls</code> - might not allow NULLs
     *    <li><code>columnNullable</code> - definitely allows NULLs
     *    <li><code>columnNullableUnknown</code> - nullability is unknown
     *    </ul>
     * <li><b>REMARKS</b> - string - comments on the column (can be NULL)
     * <li><b>COLUMN_DEF</b> - string - default value (can be NULL)
     * <li><b>SQL_DATA_TYPE</b> - short -
     * <li><b>SQL_DATETIME_SUB</b> - short -
     * <li><b>CHAR_OCTET_LENGTH</b> - int - for character data types the maximum
     * number of bytes in the column
     * <li><b>ORDINAL_POSITION</b> - int - 1-based index in the table
     * <li><b>IS_NULLABLE</b> - string - <code>"NO"</code> means in no way
     * nullable, <code>"YES"</code> means possibly nullable. Empty string means
     * nobody knows.
     * </ol>
     */
    ResultSet* getColumns(const ODBCXX_STRING& catalog,
			  const ODBCXX_STRING& schemaPattern,
			  const ODBCXX_STRING& tableNamePattern,
			  const ODBCXX_STRING& columnNamePattern);


    /** Fetches the available tables in the data source.
     * The returned ResultSet has the following columns:
     * <ol>
     * <li><b>TABLE_CAT</b> - string - table catalog (can be NULL)
     * <li><b>TABLE_SCHEM</b> - string - table schema (can be NULL)
     * <li><b>TABLE_NAME</b> - string - table name
     * <li><b>TABLE_TYPE</b> - string - table type
     * <li><b>REMARKS</b> - string - comments on the table
     * </ol>
     * @param catalog the catalog name
     * @param schemaPattern schema name search pattern
     * @param tableNamePattern table name search pattern
     * @param types a list of table types. An empty list returns all table types.
     */
    ResultSet* getTables(const ODBCXX_STRING& catalog,
			 const ODBCXX_STRING& schemaPattern,
			 const ODBCXX_STRING& tableNamePattern,
			 const std::vector<ODBCXX_STRING>& types);

    /** Fetches a list of access rights for tables in a catalog.
     *
     * A table privilege applies to one or more columns in a table.
     * Do not assume that this privilege is valid for all columns.
     *
     * The returned ResultSet is ordered by
     * <code>TABLE_CAT</code>, <code>TABLE_SCHEM</code>, <code>TABLE_NAME</code>
     * and <code>PRIVILEGE</code>. It contains the following columns:
     * <ol>
     * <li><b>TABLE_CAT</b> - string - table catalog (can be <code>NULL</code>)
     * <li><b>TABLE_SCHEM</b> - string - table schema (can be <code>NULL</code>)
     * <li><b>TABLE_NAME</b> - string - table name
     * <li><b>GRANTOR</b> - string - grantor (can be <code>NULL</code>). If
     *    <code>GRANTEE</code> owns the object, <code>GRANTOR</code> is
     *    <code>"SYSTEM"</code>.
     * <li><b>GRANTEE</b> - string - grantee
     * <li><b>PRIVILEGE</b> - string - one of <code>"SELECT"</code>,
     *     <code>"INSERT"</code>, <code>"UPDATE"</code>, <code>"DELETE"</code>,
     *     <code>"REFERENCES"</code> or a data source specific value
     * <li><b>IS_GRANTABLE</b> - string - <code>"YES"</code> if <code>GRANTEE</code>
     *     can grant this privilege to other users. <code>"NO"</code> if not.
     *     <code>NULL</code> if unknown.
     * </ol>
     */
    ResultSet* getTablePrivileges(const ODBCXX_STRING& catalog,
				  const ODBCXX_STRING& schemaPattern,
				  const ODBCXX_STRING& tableNamePattern);


    /** Fetches a list of access rights for a table's columns.
     *
     * The returned ResultSet is ordered by
     * <code>COLUMN_NAME</code> and <code>PRIVILEGE</code>.
     * It contains the following columns:
     * <ol>
     * <li><b>TABLE_CAT</b> - string - table catalog (can be <code>NULL</code>)
     * <li><b>TABLE_SCHEM</b> - string - table schema (can be <code>NULL</code>)
     * <li><b>TABLE_NAME</b> - string - table name
     * <li><b>COLUMN_NAME</b> - string - column name
     * <li><b>GRANTOR</b> - string - grantor (can be <code>NULL</code>). If
     *    <code>GRANTEE</code> owns the object, <code>GRANTOR</code> is
     *    <code>"SYSTEM"</code>.
     * <li><b>GRANTEE</b> - string - grantee
     * <li><b>PRIVILEGE</b> - string - one of <code>"SELECT"</code>,
     *     <code>"INSERT"</code>, <code>"UPDATE"</code>, <code>"DELETE"</code>,
     *     <code>"REFERENCES"</code> or a data source specific value
     * <li><b>IS_GRANTABLE</b> - string - <code>"YES"</code> if <code>GRANTEE</code>
     *     can grant this privilege to other users. <code>"NO"</code> if not.
     *     <code>NULL</code> if unknown.
     * </ol>
     */
    ResultSet* getColumnPrivileges(const ODBCXX_STRING& catalog,
				   const ODBCXX_STRING& schema,
				   const ODBCXX_STRING& table,
				   const ODBCXX_STRING& columnNamePattern);

    /** Fetches a list of primary keys for a table.
     *
     * The returned ResultSet is ordered by
     * <code>TABLE_CAT</code>, <code>TABLE_SCHEM</code>, <code>TABLE_NAME</code>
     * and <code>KEY_SEQ</code>. It contains the following columns:
     * <ol>
     * <li><b>TABLE_CAT</b> - string - table catalog (can be <code>NULL</code>)
     * <li><b>TABLE_SCHEM</b> - string - table schema (can be <code>NULL</code>)
     * <li><b>TABLE_NAME</b> - string - table name
     * <li><b>COLUMN_NAME</b> - string - column name
     * <li><b>KEY_SEQ</b> - string - sequence number in primary key (1-based)
     * <li><b>PK_NAME</b> - string - primary key (constraint) name.
     *     Can be <code>NULL</code>.
     * </ol>
     */
    ResultSet* getPrimaryKeys(const ODBCXX_STRING& catalog,
			      const ODBCXX_STRING& schema,
			      const ODBCXX_STRING& table);


    /** Fetches a list of indices and statistics for a table.
     *
     * The returned ResultSet is ordered by
     * <code>NON_UNIQUE</code>, <code>TYPE</code>, <code>INDEX_QUALIFIER</code>,
     * <code>INDEX_NAME</code> and <code>ORDINAL_POSITION</code>.
     * It contains the following columns:
     * <ol>
     * <li><b>TABLE_CAT</b> - string - table catalog (can be <code>NULL</code>)
     * <li><b>TABLE_SCHEM</b> - string - table schema (can be <code>NULL</code>)
     * <li><b>TABLE_NAME</b> - string - table name
     * <li><b>NON_UNIQUE</b> - bool - <code>true</code> if index values can
     *     be non-unique. <code>NULL</code> if <code>TYPE</code> is
     *     <code>tableIndexStatistic</code>
     * <li><b>INDEX_QUALIFIER</b> - string - index catalog, <code>NULL</code>
     *     if <code>TYPE</code> is <code>tableIndexStatistic</code>
     * <li><b>INDEX_NAME</b> - string - index name, <code>NULL</code>
     *     if <code>TYPE</code> is <code>tableIndexStatistic</code>
     * <li><b>TYPE</b> - short - index type:
     *     <ul>
     *     <li><code>tableIndexStatistic</code> - no real index - a statistic for the table
     *     <li><code>tableIndexClustered</code> - this index is clustered
     *     <li><code>tableIndexHashed</code> - this index is hashed
     *     <li><code>tableIndexOther</code> - this is some other kind of index
     *     </ul>
     * <li><b>ORDINAL_POSITION</b> - short - column sequence number in index (1-based).
     *     <code>NULL</code> if <code>TYPE</code> is <code>tableIndexStatistic</code>.
     * <li><b>COLUMN_NAME</b> - string - column name.
     *     <code>NULL</code> if <code>TYPE</code> is <code>tableIndexStatistic</code>.
     * <li><b>ASC_OR_DESC</b> - string - <code>"A"</code> for ascending,
     *     <code>"D"</code> for descending index. <code>NULL</code> if <code>TYPE</code>
     *     is <code>tableIndexStatistic</code>.
     * <li><b>CARDINALITY</b> - int - If <code>TYPE</code> is
     *     <code>tableIndexStatistic</code>, the number of rows in the
     *     table. Otherwise, the number of unique values in the index.
     * <li><b>PAGES</b> - int - Number of pages used for the table if
     *     <code>TYPE</code> is <code>tableIndexStatistic</code>. Otherwise
     *     the number of pages used for the index.
     * <li><b>FILTER_CONDITION</b> - string - filter condition (if any)
     * </ol>
     * @param catalog the catalog name
     * @param schema the schema name
     * @param table the table name
     * @param unique whether only unique indices should be looked at
     * @param approximate whether only accurate values should be retrieved
     */
    ResultSet* getIndexInfo(const ODBCXX_STRING& catalog,
			    const ODBCXX_STRING& schema,
			    const ODBCXX_STRING& table,
			    bool unique, bool approximate);

    /** Figures out in which way a foreign key table references a
     * primary key table. Returns it's findings in a ResultSet, ordered
     * by <code>FKTABLE_CAT</code>, <code>FKTABLE_SCHEM</code>,
     * <code>FKTABLE_NAME</code> and <code>KEY_SEQ</code>. The
     * ResultSet contains the following columns:
     * <ol>
     * <li><b>PKTABLE_CAT</b> - string - primary key table catalog
     * <li><b>PKTABLE_SCHEM</b> - string - primary key table schema
     * <li><b>PKTABLE_NAME</b> - string - primary key table name
     * <li><b>PKCOLUMN_NAME</b> - string - primary key column name
     * <li><b>FKTABLE_CAT</b> - string - foreign key table catalog
     * <li><b>FKTABLE_SCHEM</b> - string - foreign key table schema
     * <li><b>FKTABLE_NAME</b> - string - foreign key table name
     * <li><b>FKCOLUMN_NAME</b> - string - foreign key column name
     * <li><b>KEY_SEQ</b> - short - column sequence number in key (1-based)
     * <li><b>UPDATE_RULE</b> - short - what happens to the foreign key
     * when the primary is updated:
     *     <ul>
     *     <li><code>importedKeyNoAction</code> - nothing happends since
     *     the primary key can not be updated
     *     <li><code>importedKeyCascade</code> - change imported key to
     *     match the primary key
     *     <li><code>importedKeySetNull</code> - update the imported key to
     *     <code>NULL</code>
     *     <li><code>importedKeySetDefault</code> - update the impored key to
     *     it's default value
     *     <li><code>importedKeyRestrict</code> - same as
     *     <code>importedKeyNoAction</code>
     *     </ul>
     * <li><b>DELETE_RULE</b> - short - what happens to the foreign key
     * when the primary is deleted:
     *     <ul>
     *     <li><code>importedKeyNoAction</code> - nothing happends since
     *     the primary key can not be deleted
     *     <li><code>importedKeyCascade</code> - imported key is deleted as well
     *     <li><code>importedKeySetNull</code> - imported key is set
     *     to <code>NULL</code>
     *     <li><code>importedKeySetDefault</code> - imported key is set to it's
     *     default value
     *     <li><code>importedKeyRestrict</code> - same as
     *     <code>importedKeyNoAction</code>
     *     </ul>
     * </ol>
     */
    ResultSet* getCrossReference(const ODBCXX_STRING& primaryCatalog,
				 const ODBCXX_STRING& primarySchema,
				 const ODBCXX_STRING& primaryTable,
				 const ODBCXX_STRING& foreignCatalog,
				 const ODBCXX_STRING& foreignSchema,
				 const ODBCXX_STRING& foreignTable);

    /** Fetches a list of columns that are foreign keys to other tables'
     * primary keys.
     *
     * The returned ResultSet is identical to the one
     * returned by getCrossReference(), except it's ordered by
     * <code>PKTABLE_CAT</code>, <code>PKTABLE_SCHEM</code>,
     * <code>PKTABLE_NAME</code> and <code>KEY_SEQ</code>.
     */
    ResultSet* getImportedKeys(const ODBCXX_STRING& catalog,
			       const ODBCXX_STRING& schema,
			       const ODBCXX_STRING& table) {
      return this->getCrossReference(ODBCXX_STRING_CONST(""),
                                     ODBCXX_STRING_CONST(""),
                                     ODBCXX_STRING_CONST(""),
                                     catalog,
                                     schema,
                                     table);
    }

    /** Fetches a list of columns that reference a table's primary
     * keys.
     *
     * The returned ResultSet is identical to the one returned
     * by getCrossReference().
     */
    ResultSet* getExportedKeys(const ODBCXX_STRING& catalog,
			       const ODBCXX_STRING& schema,
			       const ODBCXX_STRING& table) {
      return this->getCrossReference(catalog,
                                     schema,
                                     table,
                                     ODBCXX_STRING_CONST(""),
                                     ODBCXX_STRING_CONST(""),
                                     ODBCXX_STRING_CONST(""));
    }

    /** Returns available procedures in a catalog.
     *
     * The returned ResultSet is ordered by
     * <code>PROCEDURE_CAT</code>, <code>PROCEDURE_SCHEM</code> and
     * <code>PROCEDURE_NAME</code>. It contains the following columns:
     * <ol>
     * <li><b>PROCEDURE_CAT</b> - string - the procedure catalog
     * <li><b>PROCEDURE_SCHEM</b> - string - the procedure schema
     * <li><b>PROCEDURE_NAME</b> - string - the procedure name
     * <li><b>NUM_INPUT_PARAMS</b> - reserved for future use
     * <li><b>NUM_OUTPUT_PARAMS</b> - reserved for future use
     * <li><b>NUM_RESULT_SETS</b> - reserved for future use
     * <li><b>REMARKS</b> - string - comments on the procedure
     * <li><b>PROCEDURE_TYPE</b> - short - the procedure type:
     *     <ul>
     *     <li><code>procedureResultUnknown</code> - can possibly return
     *     a result, but nobody is sure
     *     <li><code>procedureNoResult</code> - does not return a result
     *     <li><code>procedureReturnsResult</code> - returns a result
     *     </ul>
     * </ol>
     */
    ResultSet* getProcedures(const ODBCXX_STRING& catalog,
			     const ODBCXX_STRING& schemaPattern,
			     const ODBCXX_STRING& procedureNamePattern);

    /** Returns available procedure columns in a catalog.
     *
     * The returned ResultSet is ordered by
     * <code>PROCEDURE_CAT</code>, <code>PROCEDURE_SCHEM</code>,
     * <code>PROCEDURE_NAME</code> and <code>COLUMN_NAME</code>.
     * It contains the following columns:
     * <ol>
     * <li><b>PROCEDURE_CAT</b> - string - the procedure catalog
     * <li><b>PROCEDURE_SCHEM</b> - string - the procedure schema
     * <li><b>PROCEDURE_NAME</b> - string - the procedure name
     * <li><b>COLUMN_NAME</b> - string - the column name
     * <li><b>COLUMN_TYPE</b> - short - the column type
     *     <ul>
     *     <li><code>procedureColumnUnknown</code> - beats the driver
     *     <li><code>procedureColumnIn</code> - <code>IN</code> parameter
     *     <li><code>procedureColumnInOut</code> - <code>IN OUT</code> parameter
     *     <li><code>procedureColumnOut</code> - <code>OUT</code> parameter
     *     <li><code>procedureColumnReturn</code> - procedure return value
     *     (eg. this procedure is actually a function)
     *     <li><code>procedureColumnResult</code> - this column is part of a
     *     ResultSet this procedure returns
     *     </ul>
     * <li><b>DATA_TYPE</b> - int - SQL type of the column
     * <li><b>TYPE_NAME</b> - string - native type name
     * <li><b>COLUMN_SIZE</b> - int - the precision of the column
     * <li><b>BUFFER_LENGTH</b> - int - nothing of interest
     * <li><b>DECIMAL_DIGITS</b> - short - scale, if applicable
     * <li><b>NUM_PREC_RADIX</b> - short - radix, if applicable
     * <li><b>NULLABLE</b> - short - whether the column is nullable
     *     <ul>
     *     <li><code>procedureNoNulls</code> - not nullable
     *     <li><code>procedureNullable</code> - nullable
     *     <li><code>procedureNullableUnknown</code> - nobody knows
     *     </ul>
     * <li><b>REMARKS</b> - string - comments on the column
     * </ol>
     *
     * Note: more columns can be returned depending on the driver.
     */
    ResultSet* getProcedureColumns(const ODBCXX_STRING& catalog,
				   const ODBCXX_STRING& schemaPattern,
				   const ODBCXX_STRING& procedureNamePattern,
				   const ODBCXX_STRING& columnNamePattern);


    /** Returns the optimal set of columns that identifies a row.
     *
     * The returned ResultSet is ordered by <code>SCOPE</code> and has
     * the following columns:
     * <ol>
     * <li><b>SCOPE</b> - short - scope of this result:
     *     <ul>
     *     <li><code>bestRowTemporary</code> - temporary, only while
     *     a ResultSet is using the row
     *     <li><code>bestRowTransaction</code> - valid until the
     *     current transaction ends
     *     <li><code>bestRowSession</code> - valid through the whole
     *     session - until the connection is closed
     *     </ul>
     * <li><b>COLUMN_NAME</b> - string - the column name
     * <li><b>DATA_TYPE</b> - short - the type of the column from Types
     * <li><b>TYPE_NAME</b> - string - native type name (data source dependent)
     * <li><b>COLUMN_SIZE</b> - int - the size (precision) of the column.
     * <li><b>BUFFER_LENGTH</b> - int - unused
     * <li><b>DECIMAL_DIGITS</b> - short - scale if applicable.
     *     Can be <code>NULL</code>.
     * <li><b>PSEUDO_COLUMN</b> - short - whether this is a pseudo column:
     *     <ul>
     *     <li><code>bestRowUnknown</code> - it is unknown whether this is a pseudo
     *         column
     *     <li><code>bestRowNotPseudo</code> - it is definitely not a pseudo column
     *     <li><code>bestRowPseudo</code> - it is definitely a pseudo column
     *     </ul>
     * </ol>
     * @param catalog the catalog name
     * @param schema the schema name
     * @param table the table name
     * @param scope the scope of interest, same values as the <b>SCOPE</b> column.
     * @param nullable whether nullable columns should be included
     */
    ResultSet* getBestRowIdentifier(const ODBCXX_STRING& catalog,
				    const ODBCXX_STRING& schema,
				    const ODBCXX_STRING& table,
				    int scope,
				    bool nullable);

    /** Returns a list of columns for a table that are automatically updated
     * when anything in a row is updated.
     *
     * The returned ResultSet has the following unordered columns:
     * <ol>
     * <li><b>SCOPE</b> - short - unused
     * <li><b>COLUMN_NAME</b> - string - the column name
     * <li><b>DATA_TYPE</b> - short - the type of the column from Types
     * <li><b>TYPE_NAME</b> - string - native type name (data source dependent)
     * <li><b>COLUMN_SIZE</b> - int - the size (precision) of the column.
     * <li><b>BUFFER_LENGTH</b> - int - unused
     * <li><b>DECIMAL_DIGITS</b> - short - scale if applicable.
     *     Can be <code>NULL</code>.
     * <li><b>PSEUDO_COLUMN</b> - short - whether this is a pseudo column:
     *     <ul>
     *     <li><code>versionColumnUnknown</code> - it is unknown whether this is a pseudo
     *         column
     *     <li><code>versionColumnNotPseudo</code> - it is definitely not a pseudo column
     *     <li><code>versionColumnPseudo</code> - it is definitely a pseudo column
     *     </ul>
     * </ol>
     */
    ResultSet* getVersionColumns(const ODBCXX_STRING& catalog,
				 const ODBCXX_STRING& schema,
				 const ODBCXX_STRING& table);
				


    /** Fetches the table types the database supports.
     *
     * The returned ResultSet is the same as with getTables(), except
     * that all columns but <b>TABLE_TYPE</b> contain NULL values.
     */
    ResultSet* getTableTypes();

    /** Returns a list of available schemas in the database.
     *
     * The returned ResultSet is the same as with getTables(), except
     * that all columns but <b>TABLE_SCHEM</b> contain NULL values.
     */
    ResultSet* getSchemas();

    /** Returns a list of available catalogs in the database.
     *
     * The returned ResultSet is the same as with getTables(), except
     * that all columns but <b>TABLE_CAT</b> are NULL values.
     */
    ResultSet* getCatalogs();
  };


} // namespace odbc


#endif // __ODBCXX_DATABASEMETADATA_H
