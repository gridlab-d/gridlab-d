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

#ifndef __ODBCXX_DRIVERMANAGER_H
#define __ODBCXX_DRIVERMANAGER_H

#include <odbc++/setup.h>

#include <odbc++/types.h>

/** The namespace where all <b>libodbc++</b> classes reside */
namespace odbc {

  class Connection;
  class ErrorHandler;

  /** An ODBC Driver with it's information.
   */
  class ODBCXX_EXPORT Driver {
  private:
    ODBCXX_STRING description_;
    std::vector<ODBCXX_STRING> attributes_;

    Driver(const Driver&); //forbid
    Driver& operator=(const Driver&); //forbid

  public:
    /** Constructor */
    Driver(const ODBCXX_STRING& description,
           const std::vector<ODBCXX_STRING>& attributes)
      :description_(description), attributes_(attributes) {}

    /** Destructor */
    virtual ~Driver() {}

    /** Return a description of the driver */
    const ODBCXX_STRING& getDescription() const {
      return description_;
    }

    /** Return a list of keys that can appear in a connect string using this driver */
    const std::vector<ODBCXX_STRING>& getAttributes() const {
      return attributes_;
    }
  };

  /** A list of Drivers. Actually an STL vector */
  typedef CleanVector<Driver*> DriverList;


  /** A Data Source */
  class ODBCXX_EXPORT DataSource {
  private:
    ODBCXX_STRING name_;
    ODBCXX_STRING description_;

  public:
    /** Constructor */
    DataSource(const ODBCXX_STRING& name, const ODBCXX_STRING& description)
      :name_(name), description_(description) {}

    /** Destructor */
    virtual ~DataSource() {}

    /** Return the name of the data source */
    const ODBCXX_STRING& getName() const {
      return name_;
    }

    /** Return the description (if any) of the datasource */
    const ODBCXX_STRING& getDescription() const {
      return description_;
    }
  };

  /** A list of datasources. Behaves like an std::vector */
  typedef CleanVector<DataSource*> DataSourceList;


  /** The DriverManager.
   */
  class ODBCXX_EXPORT DriverManager {
  private:
    static SQLHENV henv_;
    static ErrorHandler* eh_;
    static int loginTimeout_;

    static void _checkInit();
    static Connection* _createConnection();

    // forbid
    DriverManager();

  public:

    /** Opens a connection by it's DSN, a username and a password */
    static Connection* getConnection(const ODBCXX_STRING& dsn,
                                     const ODBCXX_STRING& user,
                                     const ODBCXX_STRING& password);

    /** Opens a connection using an ODBC connect string.
     * @param connectString Usually something like <tt>"DSN=db;uid=user;pwd=password"</tt>
     */
    static Connection* getConnection(const ODBCXX_STRING& connectString);

    /** Gets the current login timeout in seconds.
     * @return The current login timeout in seconds, or 0 if disabled.
     */
    static int getLoginTimeout();

    /** Sets the login timeout in seconds
     * @param seconds The number of seconds to wait for a connection
     * to open. Set to 0 to disable.
     */
    static void setLoginTimeout(int seconds);

    /** Fetch a list of all available data sources
     */
    static DataSourceList* getDataSources();

    /** Fetch a list of the available drivers
     */
    static DriverList* getDrivers();

    /** Should be called before an application is to exit
     * and after all connections have been closed.
     */
    static void shutdown();
  };



} // namespace odbc


#endif // __ODBCXX_DRIVERMANAGER_H
