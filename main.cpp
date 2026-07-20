#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <algorithm>
#include <cstring>

using namespace std;

const char* DATA_FILE = "database.dat";

// Record structure for file storage
struct Record {
    char key[65];
    int value;
    bool deleted;
    
    Record() : value(0), deleted(false) {
        memset(key, 0, 65);
    }
    
    void setKey(const string& k) {
        memset(key, 0, 65);
        size_t len = min(k.length(), (size_t)64);
        memcpy(key, k.c_str(), len);
        key[len] = '\0';
    }
};

// File-based database with minimal in-memory storage
// Only keeps current command data in memory, reads/writes directly from file
class FileDatabase {
private:
    void writeRecord(fstream& file, const Record& rec, streampos pos) {
        file.seekp(pos);
        file.write(reinterpret_cast<const char*>(&rec), sizeof(Record));
    }
    
    Record readRecord(fstream& file, streampos pos) {
        Record rec;
        file.seekg(pos);
        file.read(reinterpret_cast<char*>(&rec), sizeof(Record));
        return rec;
    }
    
public:
    FileDatabase() {
        // Create file if doesn't exist
        ifstream test(DATA_FILE, ios::binary);
        if (!test.is_open()) {
            ofstream create(DATA_FILE, ios::binary);
            create.close();
        } else {
            test.close();
        }
    }
    
    void insert(const string& key, int value) {
        // Check if already exists
        fstream file(DATA_FILE, ios::in | ios::out | ios::binary);
        if (!file.is_open()) return;
        
        Record rec;
        streampos pos = 0;
        bool found = false;
        
        while (file.read(reinterpret_cast<char*>(&rec), sizeof(Record))) {
            if (!rec.deleted && string(rec.key) == key && rec.value == value) {
                found = true;
                break;
            }
            pos = file.tellg();
        }
        
        if (found) {
            file.close();
            return; // Already exists
        }
        
        // Append new record
        file.clear();
        file.seekp(0, ios::end);
        Record newRec;
        newRec.setKey(key);
        newRec.value = value;
        newRec.deleted = false;
        file.write(reinterpret_cast<const char*>(&newRec), sizeof(Record));
        file.close();
    }
    
    void remove(const string& key, int value) {
        fstream file(DATA_FILE, ios::in | ios::out | ios::binary);
        if (!file.is_open()) return;
        
        Record rec;
        streampos pos = 0;
        
        while (file.read(reinterpret_cast<char*>(&rec), sizeof(Record))) {
            if (!rec.deleted && string(rec.key) == key && rec.value == value) {
                // Mark as deleted
                rec.deleted = true;
                writeRecord(file, rec, pos);
                break;
            }
            pos = file.tellg();
        }
        
        file.close();
    }
    
    void find(const string& key) {
        fstream file(DATA_FILE, ios::in | ios::binary);
        if (!file.is_open()) {
            cout << "null" << endl;
            return;
        }
        
        vector<int> values;
        values.reserve(100); // Small reserve to avoid too many allocations
        
        Record rec;
        while (file.read(reinterpret_cast<char*>(&rec), sizeof(Record))) {
            if (!rec.deleted && string(rec.key) == key) {
                values.push_back(rec.value);
            }
        }
        
        file.close();
        
        if (values.empty()) {
            cout << "null" << endl;
        } else {
            sort(values.begin(), values.end());
            for (size_t i = 0; i < values.size(); i++) {
                if (i > 0) cout << " ";
                cout << values[i];
            }
            cout << endl;
        }
    }
};

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    
    FileDatabase db;
    
    int n;
    cin >> n;
    cin.ignore();
    
    string line, cmd, key;
    int value;
    
    for (int i = 0; i < n; i++) {
        getline(cin, line);
        istringstream iss(line);
        iss >> cmd;
        
        if (cmd == "insert") {
            iss >> key >> value;
            db.insert(key, value);
        } else if (cmd == "delete") {
            iss >> key >> value;
            db.remove(key, value);
        } else if (cmd == "find") {
            iss >> key;
            db.find(key);
        }
    }
    
    return 0;
}
