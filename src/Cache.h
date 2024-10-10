#ifndef CACHE_H
#define CACHE_H
#include <cstdint> 
#include <vector>
#include <string>
#include <cmath>
#include <queue>
#include <deque>
#include <list>

#define ADDRESSBITS 32

typedef 
struct {
   uint32_t BLOCKSIZE;
   uint32_t L1_SIZE;
   uint32_t L1_ASSOC;
   uint32_t L2_SIZE;
   uint32_t L2_ASSOC;
   uint32_t PREF_N;
   uint32_t PREF_M;
} cache_params_t;

class cache;

class cache_block{
   public:
   bool valid;          // 0 == invalid, 1 == valid
   uint32_t data;       // Tag + Index + Block Offset       
   bool dirty;          // 0 == clean 1 == dirty
   uint32_t LRUCounter;
   int tag;     // Tag

   // Default constructor
    cache_block(uint32_t blocksize, uint32_t LRUmax){
        this->valid = false;       // Blocks initialize as invalid      
        this->data = 0;
        this->tag = 0x0;   
        this->dirty = 0;
        this->LRUCounter = LRUmax;     // Initialize blocks with LRU counter = 3 (next to replace)
    }

};

class cache_set{
    public:
    std::vector<cache_block> set;
    uint32_t LRU_max;

    // Constructor that associates blocks in a set
    // Constructor
    cache_set(uint32_t assoc, uint32_t blocksize){
        this->LRU_max = assoc - 1;
        for (int i = 0; i < (int)assoc; i++){
            set.emplace_back(blocksize, LRU_max);
    }
}

    // Getter
    std::vector<cache_block>* getCacheSet();
    uint32_t getLRU_max();
};

class streamBlock {
public:
    uint32_t address;   // Address of the block

    // Default Constructor
    streamBlock() : address(0) {};

    // Constructor 
    streamBlock(uint32_t addr) : address(addr) {};
};

class streamBuffer {
public:
    int lruStreamBuffer;
    bool valid;     // 0 = Empty, 1 = Full
    std::deque<streamBlock> streamQueue;
    int size;

    // Default Constructor
    streamBuffer() {
        this->valid = false;
        this->lruStreamBuffer = 0;
        this->size = 0;
    }

    // Constructor
    streamBuffer(int lruValue, int size) {
        this->valid = false;
        //this->lruStreamBuffer = lruValue;
        this->streamQueue.resize(size);
        this->size = size;
    }
};

class prefetchUnit {
public:
    int N;  // Number of stream buffers
    int M;  // Blocks in each stream buffer
    uint32_t tempAddr;  // New member variable
    std::vector<streamBuffer> streamBuffers;  // vector of stream buffers

    // Default Constructor
    prefetchUnit() : N(0), M(0), tempAddr(0) {
        this->M = 0;
        this->tempAddr = 0;
    }

    // Constructor
    prefetchUnit(uint32_t numBuffers, uint32_t blocksPerBuffer) 
        : N(numBuffers), M(blocksPerBuffer), tempAddr(0) {
        // Initialize Stream Buffers
        for (int i = 0; i < N; ++i) {
            streamBuffers.emplace_back(i, M);  // Using streamBuffer constructor that takes capacity
            streamBuffers[i].lruStreamBuffer = N - i - 1;
        }
    }

};

class cache{
    public:
    // Cache Configuration
    std::vector<cache_set> cache_array;
    uint32_t cache_size;
    uint32_t assoc;
    uint32_t blocksize;
    uint32_t tag_bit_size;
    uint32_t index_bit_size;
    uint32_t blockoffset_size;

    // Cache Measurements
    uint32_t reads;
    uint32_t writes;
    uint32_t read_miss_count;
    uint32_t write_miss_count;
    uint32_t memory_traffic;
    uint32_t prefetches;
    uint32_t reads_prefetch;
    uint32_t read_miss_prefetch;
    uint32_t writeback;     // May need to go somewhere else

    cache *level_below;
    std::string cache_name;

    // Prefetch Config
    uint32_t prefN;
    uint32_t prefM;
    prefetchUnit* prefetch_Unit;
    bool prefetch_enabled;
    // Default Constructor
    cache(){
        this->blocksize = 0;
        this->cache_size = 0;
        this->assoc = 0;
        this->prefN = 0;
        this->prefM = 0;
        this->level_below = nullptr;
        this->cache_name = "";
    }

    // Constructor
    cache(uint32_t blocksize, uint32_t cache_size, uint32_t assoc, uint32_t pref_N, uint32_t pref_M, 
    cache* level_below, std::string cache_name){
    this->blocksize = blocksize;
    this->assoc = assoc;
    this->cache_size = cache_size;
    int num_sets = cache_size / (assoc * blocksize);    // Calculate number of sets
    this->cache_array.reserve(num_sets);
    this->level_below = level_below;

    for (int i = 0; i < num_sets; i++){                 // for loop for creating sets
        this->cache_array.emplace_back(assoc, blocksize);
    } 

    this->blockoffset_size = std::log2(blocksize); // Calculate block offset bits
    this->index_bit_size = std::log2(cache_array.size());

    this->tag_bit_size = ADDRESSBITS - index_bit_size - blockoffset_size;
    this->cache_name = cache_name;

    // Initialize stat counters
    this->reads = 0;
    this->writes = 0;
    this->read_miss_count = 0;
    this->write_miss_count = 0;
    this->writeback = 0;
    this->memory_traffic = 0;
    this->prefetches = 0;
    this->reads_prefetch = 0;
    this->read_miss_prefetch = 0;

    // Prefetch Config
    if(pref_N != 0 && pref_M != 0){
        this->prefN = pref_N;
        this->prefM = pref_M;
        this->prefetch_Unit = new prefetchUnit(prefN, prefM);
        this->prefetch_enabled = true;
    } else {
        this->prefN = 10;
        this->prefM = 10;
        this->prefetch_Unit = new prefetchUnit(10, 10);
        this->prefetch_enabled = false;
    }
}

    void request(uint32_t addr, char rw);
    //parsed_addr parse_address(uint32_t addr, char rw);    // Break instruction into tag, index, and block offset
    void print_cache_size();
    uint32_t parse_tag(uint32_t addr);
    uint32_t parse_index(uint32_t addr);
    uint32_t parse_offset(uint32_t addr);
    void update_lru(uint32_t current_LRUcounter, uint32_t addr, std::vector<cache_block> *setptr, int set_index);
    void install_block(int index, uint32_t addr, std::vector<cache_block> *setptr, char rw);
    //void write_back_to_lower_level(uint32_t addr);
    void writeback_logic(uint32_t addr, uint32_t index, std::vector<cache_block> *setptr);
    void print_cache_stats();
    void print_cache_measurements();

    bool searchStreamBuffer(uint32_t addr, streamBuffer* streamBuffer);
    void initializeStreamBuffer(uint32_t addr, streamBuffer* streamBuffer);
    void updateStreamBuffer(uint32_t addr, streamBuffer* streamBuffer);
    void printStreamBuffer();
};
 


#endif