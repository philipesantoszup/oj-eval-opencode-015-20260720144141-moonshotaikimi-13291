#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <algorithm>
#include <cstring>

using namespace std;

const char* IDX_FILE = "idx.dat";
const char* DAT_FILE = "dat.dat";

#pragma pack(push, 1)
struct IndexEntry {
    char key[65];
    int value;
    int offset;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct DataEntry {
    char deleted;
};
#pragma pack(pop)

void ensureFiles() {
    ifstream f1(IDX_FILE, ios::binary);
    if (!f1.is_open()) {
        ofstream c(IDX_FILE, ios::binary);
        c.close();
    }
    ifstream f2(DAT_FILE, ios::binary);
    if (!f2.is_open()) {
        ofstream c(DAT_FILE, ios::binary);
        c.close();
    }
}

int getIdxCount() {
    ifstream f(IDX_FILE, ios::binary | ios::ate);
    if (!f.is_open()) return 0;
    int sz = f.tellg();
    f.close();
    return sz / sizeof(IndexEntry);
}

IndexEntry readIdx(int pos) {
    IndexEntry e;
    ifstream f(IDX_FILE, ios::binary);
    f.seekg(pos * sizeof(IndexEntry));
    f.read(reinterpret_cast<char*>(&e), sizeof(IndexEntry));
    f.close();
    return e;
}

int findIdx(const string& key, int value) {
    int count = getIdxCount();
    if (count == 0) return -1;
    
    int lo = 0, hi = count - 1;
    while (lo <= hi) {
        int mid = lo + (hi - lo) / 2;
        IndexEntry e = readIdx(mid);
        int cmp = strcmp(e.key, key.c_str());
        if (cmp == 0) {
            if (e.value == value) return mid;
            if (e.value < value) lo = mid + 1;
            else hi = mid - 1;
        } else if (cmp < 0) {
            lo = mid + 1;
        } else {
            hi = mid - 1;
        }
    }
    return -1;
}

pair<int, int> findKeyRange(const string& key) {
    int count = getIdxCount();
    if (count == 0) return {-1, -1};
    
    int first = -1;
    int lo = 0, hi = count - 1;
    while (lo <= hi) {
        int mid = lo + (hi - lo) / 2;
        IndexEntry e = readIdx(mid);
        int cmp = strcmp(e.key, key.c_str());
        if (cmp == 0) {
            first = mid;
            hi = mid - 1;
        } else if (cmp < 0) {
            lo = mid + 1;
        } else {
            hi = mid - 1;
        }
    }
    
    if (first == -1) return {-1, -1};
    
    int last = -1;
    lo = 0, hi = count - 1;
    while (lo <= hi) {
        int mid = lo + (hi - lo) / 2;
        IndexEntry e = readIdx(mid);
        int cmp = strcmp(e.key, key.c_str());
        if (cmp == 0) {
            last = mid;
            lo = mid + 1;
        } else if (cmp < 0) {
            lo = mid + 1;
        } else {
            hi = mid - 1;
        }
    }
    
    return {first, last};
}

vector<IndexEntry> readAllIdx() {
    vector<IndexEntry> entries;
    ifstream f(IDX_FILE, ios::binary);
    IndexEntry e;
    while (f.read(reinterpret_cast<char*>(&e), sizeof(IndexEntry))) {
        entries.push_back(e);
    }
    f.close();
    return entries;
}

void writeAllIdx(const vector<IndexEntry>& entries) {
    ofstream f(IDX_FILE, ios::binary | ios::trunc);
    for (const auto& e : entries) {
        f.write(reinterpret_cast<const char*>(&e), sizeof(IndexEntry));
    }
    f.close();
}

bool isDeleted(int offset) {
    ifstream f(DAT_FILE, ios::binary);
    f.seekg(offset);
    DataEntry d;
    f.read(reinterpret_cast<char*>(&d), sizeof(DataEntry));
    f.close();
    return d.deleted;
}

void markDeleted(int offset) {
    fstream f(DAT_FILE, ios::in | ios::out | ios::binary);
    f.seekp(offset);
    DataEntry d;
    d.deleted = 1;
    f.write(reinterpret_cast<const char*>(&d), sizeof(DataEntry));
    f.close();
}

int appendData() {
    fstream f(DAT_FILE, ios::in | ios::out | ios::binary);
    f.seekp(0, ios::end);
    int offset = f.tellp();
    DataEntry d;
    d.deleted = 0;
    f.write(reinterpret_cast<const char*>(&d), sizeof(DataEntry));
    f.close();
    return offset;
}

void doInsert(const string& key, int value) {
    int pos = findIdx(key, value);
    if (pos >= 0) {
        IndexEntry e = readIdx(pos);
        if (isDeleted(e.offset)) {
            fstream f(DAT_FILE, ios::in | ios::out | ios::binary);
            f.seekp(e.offset);
            DataEntry d;
            d.deleted = 0;
            f.write(reinterpret_cast<const char*>(&d), sizeof(DataEntry));
            f.close();
        }
        return;
    }
    
    int offset = appendData();
    
    vector<IndexEntry> entries = readAllIdx();
    IndexEntry newEntry;
    memset(newEntry.key, 0, 65);
    size_t len = min(key.length(), (size_t)64);
    memcpy(newEntry.key, key.c_str(), len);
    newEntry.key[64] = 0;
    newEntry.value = value;
    newEntry.offset = offset;
    
    auto it = entries.begin();
    while (it != entries.end()) {
        int cmp = strcmp(it->key, key.c_str());
        if (cmp > 0 || (cmp == 0 && it->value >= value)) {
            break;
        }
        ++it;
    }
    entries.insert(it, newEntry);
    writeAllIdx(entries);
}

void doDelete(const string& key, int value) {
    int pos = findIdx(key, value);
    if (pos < 0) return;
    
    IndexEntry e = readIdx(pos);
    markDeleted(e.offset);
}

void doFind(const string& key) {
    auto range = findKeyRange(key);
    if (range.first < 0) {
        cout << "null" << endl;
        return;
    }
    
    int values[1024];
    int count = 0;
    for (int i = range.first; i <= range.second; i++) {
        IndexEntry e = readIdx(i);
        if (!isDeleted(e.offset)) {
            values[count++] = e.value;
        }
    }
    
    if (count == 0) {
        cout << "null" << endl;
    } else {
        for (int i = 0; i < count; i++) {
            if (i > 0) cout << " ";
            cout << values[i];
        }
        cout << endl;
    }
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    
    ensureFiles();
    
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
            doInsert(key, value);
        } else if (cmd == "delete") {
            iss >> key >> value;
            doDelete(key, value);
        } else if (cmd == "find") {
            iss >> key;
            doFind(key);
        }
    }
    
    return 0;
}
