/**
 * Author: Julius Adorf
 */

#include "clutseg/paramsel.h"
#include "clutseg/db.h"

#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <iostream>
#include <map>
#include <utility>

using namespace std;

namespace clutseg {

    void Serializable::serialize(sqlite3* db) {
        throw ios_base::failure("Cannot serialize, not implemented.");
    }

    void Serializable::deserialize(sqlite3* db) {
        throw ios_base::failure("Cannot deserialize, not implemented.");
    }

    void ClutsegParams::serialize(sqlite3* db) {
        db_exec(db, boost::format(
            "insert into pms_clutseg (accept_threshold, ranking) values (%f, '%s');"
        ) % accept_threshold % ranking);
        id = sqlite3_last_insert_rowid(db);
    }

    void ClutsegParams::deserialize(sqlite3* db) {
        sqlite3_stmt *read;
        db_prepare(db, read, boost::format("select accept_threshold, ranking from pms_clutseg where id=%d") % id);
        db_step(read, SQLITE_ROW);
        accept_threshold = sqlite3_column_double(read, 0);
        ranking = string((const char*) sqlite3_column_text(read, 1));
        sqlite3_finalize(read);
    }

    void Paramset::serialize(sqlite3* db) {
    }

    void Paramset::deserialize(sqlite3* db) {
    }

    void Response::serialize(sqlite3* db) {
        MemberMap m;
        setMemberField(m, "value", value);
        insertOrUpdate(db, "response", m, id);
    }

    void Response::deserialize(sqlite3* db) {
        sqlite3_stmt *read;
        db_prepare(db, read, boost::format("select value from response where id=%d") % id);
        db_step(read, SQLITE_ROW);
        value = sqlite3_column_double(read, 0);
        sqlite3_finalize(read);
    }

    void Experiment::serialize(sqlite3* db) {
    }
    
    void Experiment::deserialize(sqlite3* db) {
    }

    void setMemberField(MemberMap & m, const std::string & field, float val) {
        m[field] = str(boost::format("%f") % val);
    }
    void setMemberField(MemberMap & m, const std::string & field, double val) {
        m[field] = str(boost::format("%f") % val);
    }

    void setMemberField(MemberMap & m, const std::string & field, int val) {
        m[field] = str(boost::format("%d") % val);
    }

    void setMemberField(MemberMap & m, const std::string & field, bool val) {
        m[field] = str(boost::format("%s") % val);
    }

    void setMemberField(MemberMap & m, const std::string & field, const std::string & val) {
        m[field] = val;
    }

    void insertOrUpdate(sqlite3*  & db, const std::string & table, const MemberMap & m, int64_t & id) {
        if (id > 0) {
            stringstream upd;
            
            MemberMap::const_iterator it = m.begin();
            MemberMap::const_iterator end = m.end();

            upd << "update " << table << " set ";
            while (it != end) {
                upd << it->first;
                upd << "=";
                upd << it->second;
                upd << " ";
                it++;
            }
            upd << " where id=" << id << ";";

            db_exec(db, upd.str());
        } else {
            stringstream ins;
            
            MemberMap::const_iterator it = m.begin();
            MemberMap::const_iterator end = m.end();

            ins << "insert into " << table << " (";
            for (size_t i = 0; it != end; i++) {
                ins << it->first;
                if (i < m.size() - 1) {
                    ins << ", ";
                }
                it++;
            }
            ins << ") values (";
 
            it = m.begin();
            for (size_t i = 0; it != end; i++) {
                ins << "'" << it->second << "'";
                if (i < m.size() - 1) {
                    ins << ", ";
                }
                it++;
            }
            ins << ");";

            db_exec(db, ins.str());
            id = sqlite3_last_insert_rowid(db);
        }
    }

}

