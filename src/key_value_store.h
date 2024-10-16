#pragma once
#include <string>
#include <leveldb/db.h>

class KeyValueStore {
    public:
        KeyValueStore(const std::string& dbPath);
        ~KeyValueStore();

        bool put(const std::string& key, const std::string& value);
        bool get(const std::string& key, std::string& value);
        bool del(const std::string& key);
        bool append(const std::string& key, const std::string& value);

    private:
        leveldb::DB* db;
};