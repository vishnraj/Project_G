/* -*-C++-*-
 * __BEGIN_COPYRIGHT
 * SimpleDB: C++ ODBC database API
 * Copyright (C) 2006 Eminence Technology Pty Ltd
 * Copyright (C) 2008-2010 Russell Kliese
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 * 
 * Russell Kliese can be contacted by email: russell [at] kliese [dot] id [dot] au
 * 
 * __END_COPYRIGHT
 */

/** \mainpage
 *
 * The SimpleDB API is a C++ API designed to encapsulate the ODBC API 
 * functionality in an object oriented manner.
 * The API has been tested to work with both MySql and PostGreSQL on
 * a Debian Linux platform.
 *
 * \version
 * __BEGIN_VERSION
 * 1.15RC
 * __END_VERSION
 *
 * \date
 * __BEGIN_DATE
 * Thu, 24 Jun 2010 22:49:26 +0000
 * __END_DATE
 *
 * \section Download
 * The API source can be downloaded from our
 * <a target="_top" href="http://sourceforge.net/projects/simpledb/">sourceforge project page</a> or you can get the latest source code from the <a target="_top" href="https://sourceforge.net/scm/?type=svn&group_id=129109">Simple C++ Database Subversion repository</a>.
 *
 * \section API Description
 *
 * The SimpleDB API is a C++ API designed to encapsulate
 * the ODBC API functionality in an object oriented manner.
 *
 * The API was created due to an absence of any other such
 * API that was database independent. The database
 * independence is achieved using the ODBC (Open DataBase
 * Connectivity) API.
 *
 * The API provides a Database object that can be used to
 * create instances of Query objects. The Query objects
 * are used to query a database and allow columns to be
 * bound for the query.
 *
 * The flowing column objects are currently available:
 * - a boolean column
 * - a long column and
 * - a string column.
 * The string column makes use of the
 * libstdc++ string class so you don't have to mess around
 * with malloc.
 *
 * The Database object also has some convenience member
 * functions that allow queries that return a single
 * integer or string to be executed without having to
 * create a query object or bind columns. 
 *
 * The API has been developed under Linux and is built with the GNU toolchain.
 * It has also been reported to compile with MS Visual Studio and a Visual Studio
 * project file is included.
 *
 * The API has been reported to work with MySql, PostGreSQL and
 * SQLite on a Debian Linux platform and with MS Access on Windows.
 *
 * Example usage: see \ref simple.cpp
 *
 * \section Support
 *
 * See <a target="_top" href="https://sourceforge.net/projects/simpledb/support">Support for Simple C++ Databsae API</a>.
 * 
 * \section Bugs
 *
 * \li You can 
 * <a target="_top"
 *    href="http://sourceforge.net/tracker/?group_id=129109&atid=714392">view
 *    and report bugs through sourceforge</a> or
 * \li <a target="_top"
 *        href="http://bugs.debian.org/cgi-bin/pkgreport.cgi?pkg=simpledb">View
 *      and report bugs for the Debian package</a>
 */

/** \namespace SimpleDB
 * The namespace for the Simple %Database API
 */

/** \example complete.cpp 
 * This is an example of how to use the Simple %Database API.
 * This example makes use of just about all of the functionality. 
 * The test consists of the following steps:
 *
 * \li Create the columns that will be bound to the query.
 * \li Connect to the data source.
 * \li Create a query object from the database.
 * \li Bind the columns to the query.
 * \li Execute the query.
 * \li Fetch the results of the query.
 */ 

/** \example simple.cpp 
 * This is a simple example of how to use the Simple %Database API. 
 * The test consists of the following steps:
 *
 * \li Connect to the data source.
 * \li Create a query object from the database.
 * \li Create the columns that will be bound to the query.
 * \li Bind the columns to the query.
 * \li Execute the query.
 * \li Fetch the result of the query.
 */

/** \example transactions.cpp
 * This examples shows how the ScopedTransaction object operates in different
 * situations.
 */

#include "Column.h"
#include "Query.h"
#include "Database.h"
#include "Exception.h"
#include "ScopedTransaction.h"
