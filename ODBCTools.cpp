#include "ODBCTools.h"
#include "Logger.h"

namespace ODBC {
    const char * const ERROR_STRING = "SQL errorInfo";
    const char * const DISCONNECT_ERROR_CODE = "08";

    ODBCRow::ODBCRow(const std::vector<std::pair<int, int> > & column_meta_data) 
    {
        m_length = column_meta_data.size();
        m_columns = new SimpleDB::Column *[m_length];

        int index = 0;
        for (auto i = column_meta_data.begin(); i != column_meta_data.end(); ++i)
        {
            switch(i->first) {
                case COL_ID::BOOL :
                {
                    m_columns[index] = new SimpleDB::BoolColumn;
                    break;
                }
                case COL_ID::DOUBLE :
                {
                    m_columns[index] = new SimpleDB::DoubleColumn;
                    break;
                }
                case COL_ID::LONG_INT :
                case COL_ID::STRING :
                {
                    m_columns[index] = new SimpleDB::StringColumn(i->second);
                    break;
                }
                default :
                {
                    BOOST_LOG_TRIVIAL(error) << "We are unable to construct a column for type id " << i->first << ". Not constructing row.";
                    m_valid = false; // invalid state
                    return;
                }
            }

            m_ref.push_back(m_columns[index]);
            ++index;
        }
    }

    ODBCRow::~ODBCRow() {
        for (int i = 0; i < m_length; ++i) {
            delete m_columns[i];
        }

        delete[] m_columns;
    }

    std::weak_ptr<ODBCRow> ODBCDataSet::next_row() {
        if (m_query.get()) {
            if (!m_query->fetchRow() && m_row.get()) {
                m_row.reset();
                m_query.reset();
            }   
        }
        
        return m_row;
    }

    ODBCInterface::ODBCInterface(const std::string & conn_str
        , int max_reconnects, int max_query_attempts) : 
        m_max_reconnects(max_reconnects), m_max_query_attempts(max_query_attempts)
        , m_current_query_attempts(0), m_conn_str(conn_str) 
    {
        if (!connect()) {
            BOOST_LOG_TRIVIAL(error) << "We are unable to connect to database upon intialization. Trying again once we try querying.";
            return;
        }

        BOOST_LOG_TRIVIAL(info) << "Success! We have connected to the app database."; // once we have a more official
                                                                                      // app name we will replace "app"
    }

    bool ODBCInterface::connect() {
        int attempts = 0;
        while (attempts < m_max_reconnects) {
            try {
                m_db.reset(new SimpleDB::Database(m_conn_str, SimpleDB::Database::CONNECTION_METHOD_SQLDriverConnect));
                return true;
            } catch (SimpleDB::Exception & e) { 
                BOOST_LOG_TRIVIAL(error) << "Failed to connect to database. Trying again.";
                continue;
            }

            ++attempts;
        }

        BOOST_LOG_TRIVIAL(error) << "We could not connect. Will try again later.";
        return false;
    }

    std::unique_ptr<ODBCDataSet> ODBCInterface::run_query(const std::string & query
        , const std::vector<std::pair<int, int> > & column_meta_data) 
    {
        std::unique_ptr<ODBCDataSet> data;
        data.reset(new ODBCDataSet());

        while (m_current_query_attempts < m_max_query_attempts) { 
            try {
                data->m_query.reset(new SimpleDB::Query(*m_db));
                data->m_row.reset(new ODBCRow(column_meta_data));

                if (!data->m_row->valid()) {
                    BOOST_LOG_TRIVIAL(error) << "We had a problem setting columns for this query. Not executing.";
                    data.reset(nullptr);
                    return data;
                }

                data->m_query->bind(data->m_row->m_columns, data->m_row->m_length);
                data->m_query->execute(query);

                m_current_query_attempts = 0;
                return data;
            } catch (const SimpleDB::Exception & e) {
                BOOST_LOG_TRIVIAL(error) << query;
                BOOST_LOG_TRIVIAL(error) << e.what();

                std::string error(e.what());
                std::string error_start(ERROR_STRING);
                size_t start_pos = error.find(error_start);
                start_pos += error_start.length() + 2;

                std::string query_error_code;
                (query_error_code += error[start_pos]) += error[start_pos + 1];
                BOOST_LOG_TRIVIAL(info) << "ERROR CODE: " << query_error_code;

                if (query_error_code == DISCONNECT_ERROR_CODE) {
                    BOOST_LOG_TRIVIAL(error) << "Query cannot be executed due to a disconnect. Trying again.";

                    if (connect()) {
                        BOOST_LOG_TRIVIAL(info) << "Success! We reconnected to the database. Will now attempt the query again.";
                    }  else {
                        BOOST_LOG_TRIVIAL(error) << "Disconnected due to bad query. We cannot send this query.";
                        m_current_query_attempts = 0;
                        data.reset(nullptr);
                        return data;
                    }
                }
            }

            ++m_current_query_attempts;
        }

        BOOST_LOG_TRIVIAL(error) << "We have run out query attempts. Not executing past " << m_max_query_attempts << " times.";
        m_current_query_attempts = 0;
        data.reset(nullptr);
        return data;
    }
}
