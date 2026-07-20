#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <algorithm>
#include <cstring>

using namespace std;

// Simple file-based key-value storage
// Records are stored unsorted in the file
// Find operation reads all and sorts in a small buffer

const char* DATA_FILE = "database.dat";

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

class FileDatabase {
public:
    FileDatabase() {
        ifstream test(DATA_FILE, ios::binary);
        if (!test.is_open()) {
            ofstream create(DATA_FILE, ios::binary);
            create.close();
        }
    }
    
    void insert(const string& key, int value) {
        // Check existence
        fstream file(DATA_FILE, ios::in | ios::out | ios::binary);
        if (!file.is_open()) return;
        
        Record rec;
        while (file.read(reinterpret_cast<char*>(&rec), sizeof(Record))) {
            if (!rec.deleted && string(rec.key) == key && rec.value == value) {
                file.close();
                return;
            }
        }
        
        // Append
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
                rec.deleted = true;
                file.seekp(pos);
                file.write(reinterpret_cast<const char*>(&rec), sizeof(Record));
                break;
            }
            pos = file.tellg();
        }
        file.close();
    }
    
    void find(const string& key) {
        ifstream file(DATA_FILE, ios::binary);
        if (!file.is_open()) {
            cout << "null" << endl;
            return;
        }
        
        // Read in small batches to sort
        int batch[1024];  // Fixed size batch
        int count = 0;
        
        Record rec;
        while (file.read(reinterpret_cast<char*>(&rec), sizeof(Record))) {
            if (!rec.deleted && string(rec.key) == key) {
                // Simple insertion sort into batch
                int pos = count;
                while (pos > 0 && batch[pos-1] > rec.value) {
                    batch[pos] = batch[pos-1];
                    pos--;
                }
                batch[pos] = rec.value;
                count++;
            }
        }
        file.close();
        
        if (count == 0) {
            cout << "null" << endl;
        } else {
            for (int i = 0; i < count; i++) {
                if (i > 0) cout << " ";
                cout << batch[i];
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
