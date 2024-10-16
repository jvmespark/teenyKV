#include "key_value_store.h"
#include <leveldb/db.h>
#include <iostream>

KeyValueStore::KeyValueStore(const std::string& dbPath) {
    leveldb::Options options;
    options.create_if_missing = true;
    leveldb::Status status = leveldb::DB::Open(options, dbPath, &db);
    if (!status.ok()) {
        std::cerr<<"Unable to open/create database: "<<dbPath<<std::endl;
        std::cerr<<status.ToString()<<std::endl;
    }
}

KeyValueStore::~KeyValueStore() {
    delete db;
}

bool KeyValueStore::put(const std::string& key, const std::string& value) {
    leveldb::Status status = db->Put(leveldb::WriteOptions(), key, value);
    return status.ok();
}

bool KeyValueStore::get(const std::string& key, std::string& value) {
    leveldb::Status status = db->Get(leveldb::ReadOptions(), key, &value);
    return status.ok();
}

bool KeyValueStore::del(const std::string& key) {
    leveldb::Status status = db->Delete(leveldb::WriteOptions(), key);
    return status.ok();
}

bool KeyValueStore::append(const std::string& key, const std::string& value) {
    std::string existingValue;
    if (!get(key, existingValue))
        return false;
    existingValue += value;
    return put(key, existingValue);
}