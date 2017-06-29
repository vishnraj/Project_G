#include <string>
#include <iostream>
#include <memory>

#include "../ODBCTools.h"
#include "../Logger.h"

int main()
{
	std::string conn_string("DSN=test_db;");
	ODBC::ODBCInterface db(conn_string, 3, 2);
	std::vector<std::pair<int, int> > column_meta_data;
	column_meta_data.push_back(std::pair<int, int>(ODBC::COL_ID::STRING, 100));
	
	std::unique_ptr<ODBC::ODBCDataSet> rows = db.run_query(
		"SELECT * FROM test_table;"
		, column_meta_data);

	if (!rows) {
		BOOST_LOG_TRIVIAL(error) << "We had an issue with running this query. Exiting.";
		return 1;
	}

	for (std::weak_ptr<ODBC::ODBCRow> r = rows->next_row(); !r.expired(); r = rows->next_row())
	{
		int col_index = 0;
		for (auto i = r.lock()->start(); i != r.lock()->end(); ++i) {
			switch (column_meta_data[col_index].first) {
				case ODBC::COL_ID::BOOL :
				{
					const SimpleDB::BoolColumn * const col = dynamic_cast<const SimpleDB::BoolColumn * const>(*i);
					std::cout << col->value() << std::endl;
					break;
				}	
				case ODBC::COL_ID::DOUBLE :
				{
					const SimpleDB::DoubleColumn * const col = dynamic_cast<const SimpleDB::DoubleColumn * const>(*i);
					std::cout << col->value() << std::endl;
					break;
				}
				case ODBC::COL_ID::STRING :
				{
					const SimpleDB::StringColumn * const col = dynamic_cast<const SimpleDB::StringColumn * const>(*i);
					std::cout << col->value() << std::endl;
					break;
				}
				default :
					break;
			}

			++col_index;
		}
	}

    return 0;
}