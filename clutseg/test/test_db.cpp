/**
 * Author: Julius Adorf
 */

#include <clutseg/db.h>

#include <boost/format.hpp>
#include <boost/filesystem.hpp>
#include <gtest/gtest.h>
#include <iostream>

using namespace clutseg;
using namespace std;

struct DbTest : public ::testing::Test {

    void SetUp() {
        string fn = "build/DbTest.sqlite3";
        boost::filesystem::remove(fn);
        boost::filesystem::copy_file("./data/test.sqlite3", fn);
        // be careful, eat your own dog food
        db_open(db, fn);
    }

    void TearDown() {
        db_close(db);
    }

    sqlite3 *db;
    
};

TEST_F(DbTest, InsertRow) {
    db_exec(db, "insert into pms_clutseg (accept_threshold, ranking) values (15, 'InliersRanking')"); 
    EXPECT_EQ(2, sqlite3_last_insert_rowid(db));
    db_exec(db, "insert into pms_clutseg (accept_threshold, ranking) values (15, 'InliersRanking')"); 
    EXPECT_EQ(3, sqlite3_last_insert_rowid(db));
}

TEST_F(DbTest, DeleteRow) {
    db_exec(db, "insert into pms_clutseg (accept_threshold, ranking) values (15, 'InliersRanking')"); 
    EXPECT_EQ(2, sqlite3_last_insert_rowid(db));
    db_exec(db, "delete from pms_clutseg where ranking='InliersRanking'"); 
}

TEST_F(DbTest, CreateTable) {
    db_exec(db, "create table foo (id integer primary key);"); 
}

TEST_F(DbTest, CreateTableIfNotExists) {
    db_exec(db, "create table if not exists foo (id integer primary key);"); 
    db_exec(db, "create table if not exists foo (id integer primary key);"); 
}

TEST_F(DbTest, UseBoostFormat) {
    db_exec(db, boost::format("create table %s (id integer primary key);") % "foo"); 
}


TEST_F(DbTest, FailToCreateTableIfAlreadyExists) {
    try {
        db_exec(db, "create table foo (id integer primary key);"); 
        db_exec(db, "create table foo (id integer primary key);"); 
    } catch (ios_base::failure f) {
        EXPECT_TRUE(string(f.what()).find("Error") != string::npos);
    }
}

TEST_F(DbTest, FailOnSyntaxError) {
    try {
        db_exec(db, "create TABBLE foobar (id integer primary key);"); 
    } catch (ios_base::failure f) {
        EXPECT_TRUE(string(f.what()).find("Error") != string::npos);
    }
}

TEST_F(DbTest, FailInsertIntoNonExistingTable) {
    try {
        db_exec(db, "insert into foobar values (2, 3, 4)"); 
    } catch (ios_base::failure f) {
        EXPECT_TRUE(string(f.what()).find("Error") != string::npos);
    }
}

