#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <algorithm>
#include <cstring>

using namespace std;

// Simple file-based key-value storage using sorted insertion/deletion
// Key: string (index), Value: int
// We store each key-value pair as a record

const char* DATA_FILE = "database.dat";
const char* INDEX_FILE = "index.dat";

struct Record {
    char key[65];
    int value;
    bool deleted;
    
    Record() : value(0), deleted(false) {
        memset(key, 0, 65);
    }
};

struct IndexEntry {
    char key[65];
    long long offset;
    
    IndexEntry() : offset(0) {
        memset(key, 0, 65);
    }
};

// Simple hash table for in-memory index
// We keep only the index structure in memory, not the actual data
class FileDatabase {
private:
    fstream dataFile;
    vector<pair<string, vector<pair<int, long long>>>> index; // key -> [(value, offset)]
    bool indexLoaded;
    
    void loadIndex() {
        index.clear();
        
        // Check if data file exists
        dataFile.open(DATA_FILE, ios::in | ios::binary);
        if (!dataFile.is_open()) {
            indexLoaded = true;
            return;
        }
        
        // Build index from data file
        Record rec;
        long long offset = 0;
        while (dataFile.read(reinterpret_cast<char*>(&rec), sizeof(Record))) {
            if (!rec.deleted) {
                string keyStr(rec.key);
                // Find or create entry
                bool found = false;
                for (auto& entry : index) {
                    if (entry.first == keyStr) {
                        entry.second.push_back({rec.value, offset});
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    index.push_back({keyStr, vector<pair<int, long long>>({{rec.value, offset}})});
                }
            }
            offset = dataFile.tellg();
        }
        
        // Sort values for each key
        for (auto& entry : index) {
            sort(entry.second.begin(), entry.second.end());
        }
        
        dataFile.close();
        indexLoaded = true;
    }
    
    void saveRecord(const Record& rec, long long offset) {
        fstream file;
        file.open(DATA_FILE, ios::in | ios::out | ios::binary);
        if (!file.is_open()) return;
        file.seekp(offset);
        file.write(reinterpret_cast<const char*>(&rec), sizeof(Record));
        file.close();
    }
    
public:
    FileDatabase() : indexLoaded(false) {}
    
    void insert(const string& key, int value) {
        if (!indexLoaded) loadIndex();
        
        // Check if already exists
        for (auto& entry : index) {
            if (entry.first == key) {
                for (const auto& p : entry.second) {
                    if (p.first == value) return; // Already exists
                }
            }
        }
        
        // Append to file
        fstream file;
        file.open(DATA_FILE, ios::app | ios::binary);
        long long offset = file.tellp();
        Record rec;
        strncpy(rec.key, key.c_str(), 64);
        rec.key[64] = '\0';
        rec.value = value;
        rec.deleted = false;
        file.write(reinterpret_cast<const char*>(&rec), sizeof(Record));
        file.close();
        
        // Update index
        bool found = false;
        for (auto& entry : index) {
            if (entry.first == key) {
                entry.second.push_back({value, offset});
                sort(entry.second.begin(), entry.second.end());
                found = true;
                break;
            }
        }
        if (!found) {
            index.push_back({key, vector<pair<int, long long>>({{value, offset}})});
        }
    }
    
    void remove(const string& key, int value) {
        if (!indexLoaded) loadIndex();
        
        // Find in index
        for (auto& entry : index) {
            if (entry.first == key) {
                for (auto it = entry.second.begin(); it != entry.second.end(); ++it) {
                    if (it->first == value) {
                        // Mark as deleted in file
                        Record rec;
                        memset(rec.key, 0, 65);
                        strncpy(rec.key, key.c_str(), 64);
                        rec.key[64] = '\0';
                        rec.value = value;
                        rec.deleted = true;
                        saveRecord(rec, it->second);
                        
                        // Remove from index
                        entry.second.erase(it);
                        break;
                    }
                }
                break;
            }
        }
    }
    
    void find(const string& key) {
        if (!indexLoaded) loadIndex();
        
        for (const auto& entry : index) {
            if (entry.first == key) {
                if (entry.second.empty()) {
                    cout << "null" << endl;
                    return;
                }
                for (size_t i = 0; i < entry.second.size(); i++) {
                    if (i > 0) cout << " ";
                    cout << entry.second[i].first;
                }
                cout << endl;
                return;
            }
        }
        cout << "null" << endl;
    }
};

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    
    FileDatabase db;
    
    int n;
    cin >> n;
    cin.ignore();
    
    string line;
    for (int i = 0; i < n; i++) {
        getline(cin, line);
        istringstream iss(line);
        string cmd;
        iss >> cmd;
        
        if (cmd == "insert") {
            string key;
            int value;
            iss >> key >> value;
            db.insert(key, value);
        } else if (cmd == "delete") {
            string key;
            int value;
            iss >> key >> value;
            db.remove(key, value);
        } else if (cmd == "find") {
            string key;
            iss >> key;
            db.find(key);
        }
    }
    
    return 0;
}
