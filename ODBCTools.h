#pragma once

#include <string>
#include <vector>
#include <map>

#include <memory>
#include <utility>

#include <boost/scoped_ptr.hpp>

#include "SimpleDB/SimpleDB.h"

namespace ODBC {
    enum COL_ID {BOOL, DOUBLE, STRING, LONG_INT};

    class ODBCRow {
    private:
        bool m_valid{true};
        SimpleDB::Column ** m_columns;
        int m_length;

        std::vector<SimpleDB::Column *> m_ref; 

        friend class ODBCInterface;
    public:
        ODBCRow()=delete; // in order to guarantee that the columns are known
                          // when this object is created

        // EFFECTS:
        // Constructs a row of columns given the column_meta_data:
        // (type, [size - if type=STRING|LONG_INT]). Row of length -1 indicates
        // that there was an error constructing columns
        ODBCRow(const std::vector<std::pair<int, int> > & column_meta_data);
        ~ODBCRow();

        inline std::vector<SimpleDB::Column *>::const_iterator start() {
            return m_ref.begin();
        }
        inline std::vector<SimpleDB::Column *>::const_iterator end() {
            return m_ref.end();
        }
        inline bool valid() {
            return m_valid;
        }
    };

    // Note:
    // This class has no public constructor
    // to set its memebers because it should
    // only be returned from ODBCInterface queries
    class ODBCDataSet {
    private:
        boost::scoped_ptr<SimpleDB::Query> m_query;
        std::shared_ptr<ODBCRow> m_row; // the current row in the dataset
                                        // returned by the query associated
                                        // with m_query
        friend class ODBCInterface;
    public:
        // RETURNS:
        // a pointer to the current row that
        // is pointed to by m_row - because
        // it is a weak_ptr, it will get
        // invalidated upon object destruction,
        // check weak_ptr to see if it has expired
        // if there is no more data to be read
        std::weak_ptr<ODBCRow> next_row();
    };

    class ODBCInterface {
    private:
        const int m_max_reconnects;
        const int m_max_query_attempts;
        int m_current_query_attempts;
        std::string m_conn_str;
      
        boost::scoped_ptr<SimpleDB::ScopedTransaction> m_transaction;
        boost::scoped_ptr<SimpleDB::Database> m_db;
       
        // EFFECTS:
        // True upon a successful connection, false otherwise 
        bool connect();
    public:
        ODBCInterface(const std::string & conn_str, int max_reconnects
            , int max_query_attempts);

        // EFFECTS:
        // Bind row and query to m_query
        // RETURNS:
        // a non-empty ODBCDataSet if successful, empty otherwise
        std::unique_ptr<ODBCDataSet> run_query(const std::string & query
            , const std::vector<std::pair<int, int> > & column_types);

        inline void transaction_start() {
            m_transaction.reset(new SimpleDB::ScopedTransaction(*m_db));
        }
        inline void transaction_rollback() {
            m_transaction.reset();
        }
        inline void transaction_commit() {
            m_transaction->commit();
        }
    };
}
