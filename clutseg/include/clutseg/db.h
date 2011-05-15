/**
 * Author: Julius Adorf
 *
 * Tiny abstraction layer around sqlite3. Especially, shall use exceptions
 * instead of error codes, hiding some parameters and avoiding use of 'const
 * char *' and 'const unsigned char *'.
 */

#ifndef _DB_H_
#define _DB_H_

#include <boost/filesystem.hpp>
#include <boost/format/format_class.hpp>
#include <sqlite3.h>
#include <string>

namespace clutseg {
    
    /** Delegates to sqlite3_open. Throws ios_base::failure if an error occurs. */
    void db_open(sqlite3* & db, const boost::filesystem::path & filename);
    
    /** Delegates to sqlite3_exec. Throws ios_base::failure if an error occurs. */
    void db_exec(sqlite3* & db, const std::string & sql);

    /** Delegates to sqlite3_exec. Throws ios_base::failure if an error occurs. */
    void db_exec(sqlite3* & db, boost::format & sql);
    
    void db_prepare(sqlite3* & db, sqlite3_stmt* & stmt, const std::string & sql);

    void db_prepare(sqlite3* & db, sqlite3_stmt* & stmt, const boost::format & sql);
    
    void db_step(sqlite3_stmt* & stmt, int expected_status);

    /** Delegates to sqlite3_open. Throws ios_base::failure if an error occurs. */
    void db_close(sqlite3* & db);

}

#endif
