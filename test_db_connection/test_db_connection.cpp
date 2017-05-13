#include <string>
#include <iostream>
#include "../SimpleDB/SimpleDB.h"

int main()
{
	std::string conn_string("DSN=test_db;UID=vishnraj;PWD=***REMOVED***");
	SimpleDB::Database db(conn_string, SimpleDB::Database::CONNECTION_METHOD_SQLDriverConnect);

	SimpleDB::StringColumn name(100);
	SimpleDB::Column* col[1] = {&name};
	try {
		SimpleDB::Query q(db);
		q.bind(col, 1);
		q.execute("SELECT * FROM test_table;");
		while (q.fetchRow()) {
			std::cout << name << '\n';
		}
	} catch (SimpleDB::Exception & e) {
		std::cout << e.what();
	}

    return 0;
}