#include <iostream>
#include <unordered_map>
#include <list>
#include <set>
#include <chrono>
#include <fstream>
#include <sstream>
#include <vector>
#include <algorithm>
#include <iomanip>
using namespace std;
using namespace std::chrono;

class Cache {
protected:
    int capacity;
    int hits = 0;
    int misses = 0;
    string policyName;
    
public:
    Cache(int cap, string name) : capacity(cap), policyName(name) {}
    virtual void refer(int key) = 0;
    virtual void display() = 0;
    virtual void displayStats() {
        cout << "\nCache Statistics (" << policyName << "):\n";
        cout << "----------------------------\n";
        cout << "Total accesses: " << hits + misses << "\n";
        cout << "Hits: " << hits << " (" << fixed << setprecision(2) 
             << ((float)hits/(hits+misses))*100 << "%)\n";
        cout << "Misses: " << misses << " (" << fixed << setprecision(2) 
             << ((float)misses/(hits+misses))*100 << "%)\n";
    }
    virtual void saveToFile(const string& filename) = 0;
    virtual void loadFromFile(const string& filename) = 0;
    virtual void clear() {
        hits = 0;
        misses = 0;
    }
    virtual ~Cache() {}
    
    // Benchmarking function
    void benchmark(const vector<int>& testData) {
        auto start = high_resolution_clock::now();
        
        for (int key : testData) {
            refer(key);
        }
        
        auto stop = high_resolution_clock::now();
        auto duration = duration_cast<microseconds>(stop - start);
        
        cout << "\nBenchmark Results (" << policyName << "):\n";
        cout << "----------------------------\n";
        cout << "Operations: " << testData.size() << "\n";
        cout << "Time taken: " << duration.count() << " microseconds\n";
        cout << "Operations per μs: " << testData.size()/(double)duration.count() << "\n";
    }
};

// Enhanced LRU Cache Implementation
class LRUCache : public Cache {
    list<int> dq; // doubly linked list to maintain order
    unordered_map<int, list<int>::iterator> ma; // map to store references
    unordered_map<int, int> keyAccessCount; // Track access frequency
    
public:
    LRUCache(int cap) : Cache(cap, "LRU") {}
    
    void refer(int key) override {
        // Key not in cache
        if (ma.find(key) == ma.end()) {
            misses++;
            // Cache is full
            if (dq.size() == capacity) {
                int last = dq.back();
                dq.pop_back();
                ma.erase(last);
                keyAccessCount.erase(last);
            }
        }
        // Key is in cache
        else {
            hits++;
            dq.erase(ma[key]);
        }
        
        // Update reference
        dq.push_front(key);
        ma[key] = dq.begin();
        keyAccessCount[key]++;
    }
    
    void display() override {
        cout << "LRU Cache Contents (Most recent to least recent):\n";
        for (auto it = dq.begin(); it != dq.end(); it++) {
            cout << *it << " (accessed " << keyAccessCount[*it] << " times)\n";
        }
    }
    
    void saveToFile(const string& filename) override {
        ofstream outfile(filename);
        if (outfile.is_open()) {
            outfile << "LRU Cache\n";
            outfile << "Capacity: " << capacity << "\n";
            outfile << "Contents (most recent first):\n";
            for (auto it = dq.begin(); it != dq.end(); ++it) {
                outfile << *it << "\n";
            }
            outfile.close();
        }
    }

    void clear() override {
        Cache::clear(); // Clear base statistics
        dq.clear();     // Clear the doubly-linked list
        ma.clear();     // Clear the reference map
        keyAccessCount.clear(); // Clear access counts
    }
    
    void loadFromFile(const string& filename) override {
        clear();
        ifstream infile(filename);
        string line;
        vector<int> keys; // Temporary storage for keys
        
        if (infile.is_open()) {
            // Read all keys first
            while (getline(infile, line)) {
                if (isdigit(line[0])) {
                    keys.push_back(stoi(line));
                }
            }
            infile.close();
            
            // Insert keys in reverse order to maintain LRU ordering
            for (auto it = keys.rbegin(); it != keys.rend(); ++it) {
                refer(*it);
            }
        }
    }
};

// Enhanced LFU Cache Implementation
class LFUCache : public Cache {
    struct Node {
        int key, value, freq;
        steady_clock::time_point lastAccess;
        Node(int k, int v, int f) : key(k), value(v), freq(f) {
            lastAccess = steady_clock::now();
        }
    };
    
    int minFreq;
    unordered_map<int, list<Node>::iterator> keyMap;
    unordered_map<int, list<Node>> freqMap;
    
    // Helper to get current timestamp in ms
    long getTimeStamp() {
        return duration_cast<milliseconds>(
            steady_clock::now().time_since_epoch()).count();
    }
    
public:
    LFUCache(int cap) : Cache(cap, "LFU"), minFreq(0) {}
    
    void refer(int key) override {
        if (capacity == 0) return;
        
        auto it = keyMap.find(key);
        
        // Key not present
        if (it == keyMap.end()) {
            misses++;
            // Cache is full
            if (keyMap.size() >= capacity) {
                // Find least frequently used item
                while (freqMap[minFreq].empty()) {
                    minFreq++;
                }
                
                auto& minFreqList = freqMap[minFreq];
                Node toDelete = minFreqList.back();
                keyMap.erase(toDelete.key);
                minFreqList.pop_back();
            }
            
            minFreq = 1;
            freqMap[1].push_front(Node(key, key, 1));
            keyMap[key] = freqMap[1].begin();
        }
        // Key present
        else {
            hits++;
            auto nodeIt = it->second;
            int value = nodeIt->value;
            int freq = nodeIt->freq;
            auto lastAccess = nodeIt->lastAccess;
            
            // Remove from current frequency list
            freqMap[freq].erase(nodeIt);
            if (freqMap[freq].empty()) {
                freqMap.erase(freq);
                if (minFreq == freq) minFreq++;
            }
            
            // Insert into new frequency list
            Node newNode(key, value, freq+1);
            newNode.lastAccess = lastAccess; // Preserve original access time
            freqMap[freq+1].push_front(newNode);
            keyMap[key] = freqMap[freq+1].begin();
        }
    }

    void clear() override {
        Cache::clear(); // Clear base statistics
        keyMap.clear();
        freqMap.clear();
        minFreq = 0;
    }
    
    void display() override {
        cout << "LFU Cache Contents (by frequency):\n";
        for (auto& freqNodesPair : freqMap) {
            int freq = freqNodesPair.first;
            list<Node>& nodes = freqNodesPair.second;
            
            cout << "Frequency " << freq << ":\n";
            for (auto& node : nodes) {
                auto age = duration_cast<seconds>(
                    steady_clock::now() - node.lastAccess).count();
                cout << "  " << node.key << " (last accessed " << age << "s ago)\n";
            }
        }
    }
    
    void saveToFile(const string& filename) override {
        ofstream outfile(filename);
        if (outfile.is_open()) {
            outfile << "LFU Cache\n";
            outfile << "Capacity: " << capacity << "\n";
            outfile << "Contents by frequency:\n";
            
            // Iterate through freqMap without structured bindings
            for (auto& freqEntry : freqMap) {
                int freq = freqEntry.first;
                list<Node>& nodes = freqEntry.second;
                
                outfile << "Frequency " << freq << ":\n";
                for (auto& node : nodes) {
                    outfile << "  " << node.key << "\n";
                }
            }
            outfile.close();
        }
    }
    
    void loadFromFile(const string& filename) override {
        clear(); // Clear existing cache first
        ifstream infile(filename);
        string line;
        
        if (infile.is_open()) {
            while (getline(infile, line)) {
                // Skip headers (LFU Cache, Capacity, etc.)
                if (line.empty() || line.find("Frequency") != string::npos) {
                    continue;
                }
                
                // Extract key (lines like "  2")
                if (line.find_first_of("0123456789") != string::npos) {
                    stringstream ss(line);
                    int key;
                    ss >> key; // Extract the number
                    refer(key); // Re-insert into cache
                }
            }
            infile.close();
        }
    }
};

// Cache Simulator with expanded features
void cacheSimulator() {
    int capacity, choice;
    cout << "Enter cache capacity: ";
    cin >> capacity;
    
    cout << "Choose replacement policy:\n1. LRU\n2. LFU\nEnter choice: ";
    cin >> choice;
    
    Cache* cache;
    if (choice == 1) {
        cache = new LRUCache(capacity);
    } else {
        cache = new LFUCache(capacity);
    }
    
    while (true) {
        cout << "\n1. Refer key\n2. Display cache\n3. Show statistics\n"
             << "4. Save cache to file\n5. Load cache from file\n"
             << "6. Run benchmark test\n7. Clear cache\n8. Exit\nEnter choice: ";
        cin >> choice;
        
        if (choice == 1) {
            int key;
            cout << "Enter key: ";
            cin >> key;
            cache->refer(key);
        } else if (choice == 2) {
            cache->display();
        } else if (choice == 3) {
            cache->displayStats();
        } else if (choice == 4) {
            string filename;
            cout << "Enter filename: ";
            cin >> filename;
            cache->saveToFile(filename);
        } else if (choice == 5) {
            string filename;
            cout << "Enter filename: ";
            cin >> filename;
            cache->loadFromFile(filename);
        } else if (choice == 6) {
            vector<int> testData;
            int size;
            cout << "Enter benchmark size: ";
            cin >> size;
            
            // Generate random test data
            srand(time(0));
            for (int i = 0; i < size; i++) {
                testData.push_back(rand() % 100);
            }
            
            cache->benchmark(testData);
        } else if (choice == 7) {
            cache->clear();
            cout << "Cache cleared.\n";
        } else if (choice == 8) {
            break;
        } else {
            cout << "Invalid choice!\n";
        }
    }
    
    delete cache;
}

int main() {
    cout << "Cache Simulator Project\n";
    cout << "-------------------------------\n";
    cacheSimulator();
    return 0;
}

