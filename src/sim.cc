// Project 1: Cache Simulator
// Author: Ethan Cornett
// Date: 2024-10

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <vector>
#include <cstdint> 
#include <iostream>
#include <cmath>
#include <iomanip>
#include <queue>
#include <deque>
#include <list>

#include "Cache.h"


/*  "argc" holds the number of command-line arguments.
    "argv[]" holds the arguments themselves.

    Example:
    ./sim 32 8192 4 262144 8 3 10 gcc_trace.txt
    argc = 9
    argv[0] = "./sim"
    argv[1] = "32"
    argv[2] = "8192"
    ... and so on
*/
int main (int argc, char *argv[]) {
   FILE *fp;			// File pointer.
   char *trace_file;		// This variable holds the trace file name.
   cache_params_t params;	// Look at the sim.h header file for the definition of struct cache_params_t.
   char rw;			// This variable holds the request's type (read or write) obtained from the trace.
   uint32_t addr;		// This variable holds the request's address obtained from the trace.
				// The header file <inttypes.h> above defines signed and unsigned integers of various sizes in a machine-agnostic way.  "uint32_t" is an unsigned integer of 32 bits.

   // Exit with an error if the number of command-line arguments is incorrect.
   if (argc != 9) {
      printf("Error: Expected 8 command-line arguments but was provided %d.\n", (argc - 1));
      exit(EXIT_FAILURE);
   }
    
   // "atoi()" (included by <stdlib.h>) converts a string (char *) to an integer (int).
   params.BLOCKSIZE = (uint32_t) atoi(argv[1]);
   params.L1_SIZE   = (uint32_t) atoi(argv[2]);
   params.L1_ASSOC  = (uint32_t) atoi(argv[3]);
   params.L2_SIZE   = (uint32_t) atoi(argv[4]);
   params.L2_ASSOC  = (uint32_t) atoi(argv[5]);
   params.PREF_N    = (uint32_t) atoi(argv[6]);
   params.PREF_M    = (uint32_t) atoi(argv[7]);
   trace_file       = argv[8];

   // Open the trace file for reading.
   fp = fopen(trace_file, "r");
   if (fp == (FILE *) NULL) {
      // Exit with an error if file open failed.
      printf("Error: Unable to open file %s\n", trace_file);
      exit(EXIT_FAILURE);
   }
    
   // Print simulator configuration.
   printf("===== Simulator configuration =====\n");
   printf("BLOCKSIZE:  %u\n", params.BLOCKSIZE);
   printf("L1_SIZE:    %u\n", params.L1_SIZE);
   printf("L1_ASSOC:   %u\n", params.L1_ASSOC);
   printf("L2_SIZE:    %u\n", params.L2_SIZE);
   printf("L2_ASSOC:   %u\n", params.L2_ASSOC);
   printf("PREF_N:     %u\n", params.PREF_N);
   printf("PREF_M:     %u\n", params.PREF_M);
   printf("trace_file: %s\n", trace_file);
   printf("\n");


   // Only create L2 cache if L2_SIZE != 0
   cache* L2 = NULL;
   uint32_t tempN = 0;
   uint32_t tempM = 0;
   if (params.L2_SIZE != 0){
      L2 = new cache(params.BLOCKSIZE, params.L2_SIZE, params.L2_ASSOC, params.PREF_N, params.PREF_M, NULL, "L2");
   } else {
      // If there is NOT a L2 cache then set prefetch for L1
      tempN = params.PREF_N;
      tempM = params.PREF_M;
   }
   cache L1(params.BLOCKSIZE, params.L1_SIZE, params.L1_ASSOC, tempN, tempM, L2, "L1");
   // set prefetch unit status
   if(params.PREF_N == 0 && params.PREF_M == 0){
      L1.prefetch_enabled = false;
   }

   // Read requests from the trace file and echo them back.
   while (fscanf(fp, "%c %x\n", &rw, &addr) == 2) {	// Stay in the loop if fscanf() successfully parsed two tokens as specified.
   //    if (rw == 'r')
   //       printf("r %x\n", addr);
   //    else if (rw == 'w')
   //       printf("w %x\n", addr);
   //    else {
   //       printf("Error: Unknown request type %c.\n", rw);
	//  exit(EXIT_FAILURE);
   //    }
   

      L1.request(addr, rw); 
   }
   // --------- Print final stats ---------- //
      
   L1.print_cache_stats();
   if(L2 != NULL) { (*L2).print_cache_stats(); }
   if(L1.prefetch_enabled){ L1.printStreamBuffer(); }
   if(L2 != NULL && L2->prefetch_enabled){ (*L2).printStreamBuffer(); }
   L1.print_cache_measurements();

   return(0);
}


// ------------ Class: cache_set ------------ //
std::vector<cache_block>* cache_set::getCacheSet(){
    return &this->set;
}

uint32_t cache_set::getLRU_max(){
   return this->LRU_max;
}

// ------------ Class: cache_array ------------ //
// Calculate address tag
uint32_t cache::parse_tag(uint32_t addr){
   // Shift addr
   addr = addr >> (this->index_bit_size + this->blockoffset_size);

   // Bit mask tag bits
   uint32_t tag_mask = (1 << this->tag_bit_size) - 1;  // may be the rest of the bits so this is unnecessary?
   return tag_mask & addr;
}

// Calculate address index
uint32_t cache::parse_index(uint32_t addr){
   // Shift temp_addr
   addr = addr >> this->blockoffset_size;

   // Bit mask index bits
   uint32_t index_mask = (1 << this->index_bit_size) - 1;
   return index_mask & addr;
}

// Calculate address block offset
uint32_t cache::parse_offset(uint32_t addr){
   // Bit mask block offset
   uint32_t block_offset_mask = (1 << this->blockoffset_size) - 1;
   return block_offset_mask & addr;
}

void cache::print_cache_stats(){
   std::cout << "===== " << this->cache_name << " contents =====\n";

   for (int i = 0; i < (int)this->cache_array.size(); i++) {   // Iterate over all sets
      std::cout << std::dec << "set      " << i << ":    ";

      int array_size = (int)this->assoc;        // Number of blocks in each set

      // Initialize blockArray with nullptrs
      cache_block* blockArray[array_size];
      for (int idx = 0; idx < array_size; idx++) {
          blockArray[idx] = nullptr;
      }

      // Populate blockArray based on LRUCounter
      for (int j = 0; j < (int)this->cache_array[i].set.size(); j++){
         uint32_t lru = this->cache_array[i].set[j].LRUCounter;

         // Ensure lru is within bounds
         if (lru < array_size) {
             blockArray[lru] = &this->cache_array[i].set[j];
         }
      }

      // Print the blocks in order
      for (int k = 0; k < array_size; k++) {
         cache_block* block = blockArray[k];

         if (block != nullptr && block->valid) {
             // Safe to access block
             char D = (block->dirty) ? 'D' : ' ';

             // Convert tag to hex and print
             std::cout << std::hex << this->parse_tag(block->data) << " " << D << "   ";
         }
      }
      std::cout << std::dec << std::endl; // new line for each set
   }
   std::cout << std::dec << std::endl; // new line before Measurements
}

void cache::print_cache_measurements(){
   std::cout << "===== Measurements =====" << std::endl;
   // Print measurements a - q
   std::cout << "a. " << cache_name << " reads:                   " << reads << std::endl;
   std::cout << "b. " << cache_name << " read misses:             " << read_miss_count << std::endl;
   std::cout << "c. " << cache_name << " writes:                  " << writes << std::endl;
   std::cout << "d. " << cache_name << " write misses:            " << write_miss_count << std::endl;
   
   // Calculate miss rate (read & write misses / Total addr)
   double miss_rate = static_cast<double>(write_miss_count + read_miss_count) / static_cast<double>(writes + reads);
   std::cout << "e. " << std::fixed << std::setprecision(4) << cache_name << std::dec << " miss rate:               " << miss_rate << std::endl;
   std::cout << "f. " << cache_name << " writebacks:              " << writeback << std::endl;

   // Add L1 prefetches
   std::cout << "g. " << cache_name << " prefetches:              " << prefetches << std::endl;
 
   if (this->level_below != NULL) {
       // L2 Measurements
       std::cout << "h. " << this->level_below->cache_name << " reads (demand):          " << this->level_below->reads << std::endl;
       std::cout << "i. " << this->level_below->cache_name << " read misses (demand):    " << this->level_below->read_miss_count << std::endl;
       // ADD reads (prefetch)
       std::cout << "j. " << this->level_below->cache_name << " reads (prefetch):        " << this->level_below->reads_prefetch << std::endl;
       // ADD read misses (prefetch)
       std::cout << "k. " << this->level_below->cache_name << " read misses (prefetch):  " << this->level_below->read_miss_prefetch << std::endl;

       std::cout << "l. " << this->level_below->cache_name << " writes:                  " << this->level_below->writes << std::endl;
       std::cout << "m. " << this->level_below->cache_name << " write misses:            " << this->level_below->write_miss_count << std::endl;
       
       // Calculate miss rate (read & write misses / Total addr)
       double miss_rate_2 = static_cast<double>(this->level_below->read_miss_count) / static_cast<double>(this->level_below->reads);

       std::cout << "n. " << std::fixed << std::setprecision(4) << this->level_below->cache_name << std::dec << " miss rate:               " << miss_rate_2 << std::endl;
       std::cout << "o. " << this->level_below->cache_name << " writebacks:              " << this->level_below->writeback << std::endl;

       // ADD L2 prefetches
       std::cout << "p. " << this->level_below->cache_name << " prefetches:              " << this->level_below->prefetches << std::endl;

       std::cout << "q. memory traffic:             " << this->level_below->memory_traffic << std::endl;
   } else {
       // If there's no L2 cache, adjust the measurements accordingly
       std::cout << "h. " << "L2 reads (demand):          " << 0 << std::endl;
       std::cout << "i. " << "L2 read misses (demand):    " << 0 << std::endl;
       // ADD reads (prefetch)
       std::cout << "j. " << "L2 reads (prefetch):        " << 0 << std::endl;
       // ADD read misses (prefetch)
       std::cout << "k. " << "L2 read misses (prefetch):  " << 0 << std::endl;

       std::cout << "l. " << "L2 writes:                  " << 0 << std::endl;
       std::cout << "m. " << "L2 write misses:            " << 0 << std::endl;
       
       std::cout << "n. " << std::fixed << std::setprecision(4) << "L2 miss rate:               " << 0.0000 << std::endl;
       std::cout << "o. " << "L2 writebacks:              " << 0 << std::endl;

       // ADD L2 prefetches
       std::cout << "p. " << "L2 prefetches:              "  << 0 << std::endl;

       std::cout << "q. memory traffic:             " << this->memory_traffic << std::endl;
   }
}

void cache::update_lru(uint32_t current_LRUcounter, uint32_t addr, std::vector<cache_block> *setptr, int set_index){

   // Increment the counters of other blocks in set whose 
   // counters are less than the referenced block's counter.
   for (uint32_t i = 0; i < setptr->size(); i++){
      // If LRU counter is less than the referenced blocks counter and it is not the same block being replaced then update LRU
      if(((*setptr)[i].LRUCounter < current_LRUcounter)){
         (*setptr)[i].LRUCounter++;       // Increment LRU
      }
   }
   // Update current block
   (*setptr)[set_index].LRUCounter = 0;
   return;
}

// Function to install block
void cache::install_block(int index, uint32_t addr, std::vector<cache_block> *setptr, char rw){
   // Install block
   (*setptr)[index].data = addr;                 // Update address
   (*setptr)[index].tag = this->parse_tag(addr);
   (*setptr)[index].dirty = (rw == 'w') ? true : false;    // Set dirty bit if write, else install clean
   (*setptr)[index].valid = true;                   // Mark block as valid
   this->update_lru((*setptr)[index].LRUCounter, addr, setptr, index);    // Update LRU
}

void cache::writeback_logic(uint32_t addr, uint32_t evict_index, std::vector<cache_block> *setptr){
   if((*setptr)[evict_index].dirty){   // Check if evict block is dirty, if so writeback
      // Write back to lower level before replacing
      if (this->level_below != NULL){ 
         this->level_below->request((*setptr)[evict_index].data, 'w'); 
      } else {
         this->memory_traffic++;
      }
      this->writeback++;      // Write back to memory
   }
}

bool cache::searchStreamBuffer(uint32_t addr, streamBuffer* streamBuffer){
   // Calculate search address
   uint32_t search_addr = addr >> this->blockoffset_size;

   // Reference to the stream queue for convenience
   std::deque<streamBlock>& streamQueue = streamBuffer->streamQueue;

   // Check if the stream buffer is empty
   if(streamQueue.empty()){
      // set valid bit to false
      streamBuffer->valid = false;
      return false;
   }

   // check to make sure that the stream buffer is not over the max size
   if((int)streamQueue.size() > this->prefetch_Unit->M){
      std::cout << "Stream buffer is over the max size" << std::endl;
      return false;
   }

   // Search stream buffer
   std::deque<streamBlock>::iterator it;  // Iterator for the stream buffer

   for (it = streamQueue.begin(); it != streamQueue.end(); ++it) {
      const streamBlock& block = *it;     // Get the current block
      if (block.address == search_addr) { // If the current block is the search address

         uint32_t hit_index = block.address;

         // Remove all blocks up to and including the hit block
         streamQueue.erase(streamQueue.begin(), it + 1);

         // Check if the stream buffer is empty
         if(streamQueue.empty()){
            // set valid bit to false
            streamBuffer->valid = false;
         }
         return true;
      }
   }
   return false;
}

// Initialize stream buffer
void cache::initializeStreamBuffer(uint32_t addr, streamBuffer* streamBuffer){

   bool prefetch_from_lower_cache = false;

   // Clear the stream buffer
   streamBuffer->streamQueue.clear();

   // Check if prefetch unit is not the lowest cache level
   if(this->level_below != NULL){
      std::cout << "Prefetching from lower cache" << std::endl;
      prefetch_from_lower_cache = true;
   } 

   // Create temp address
   uint32_t tempAddr = addr >> this->blockoffset_size;

   // Fill the stream buffer with new blocks starting from addr to addr + M - 1
   for (int i = 0; i < this->prefetch_Unit->M; i++) {

      // check if the stream buffer is over the max size
      if(this->prefetch_Unit->M <= (int)streamBuffer->streamQueue.size()){
         //std::cout << "Stream buffer is over the max size" << std::endl;
         break;
      }

      // Increment prefetches
      this->prefetches++;

      uint32_t newAddress = tempAddr + i + 1;
      streamBuffer->streamQueue.push_back(newAddress);

      if(prefetch_from_lower_cache){
         // Send prefetch request to lower cache
         this->level_below->request(newAddress, 'r');
      } else {
         // Send prefetch request to memory
         this->memory_traffic++;
      }
   } 
   // Set valid bit
   streamBuffer->valid = true;

   // all other stream buffers are now one less recently used
   for(int i = 0; i <= this->prefetch_Unit->N; i++){
      if(this->prefetch_Unit->streamBuffers[i].lruStreamBuffer > streamBuffer->lruStreamBuffer){
         this->prefetch_Unit->streamBuffers[i].lruStreamBuffer--;
         
         // Check to make sure that LRU value is not negative
         if(this->prefetch_Unit->streamBuffers[i].lruStreamBuffer < 0){
            this->prefetch_Unit->streamBuffers[i].lruStreamBuffer = 0;
         }
      }
   }  

   // set as most recently used
   streamBuffer->lruStreamBuffer = this->prefetch_Unit->N - 1;

   return;
}

// Update stream buffer
void cache::updateStreamBuffer(uint32_t addr, streamBuffer* streamBuffer){

   bool prefetch_from_lower_cache = false;

   // Check if prefetch unit is not the lowest cache level
   if(this->level_below != NULL){
      std::cout << "Prefetching from lower cache" << std::endl;
      prefetch_from_lower_cache = true;
   } 

   uint32_t startAddr = addr;

   // Check if stream buffer is valid
   if(streamBuffer->valid){
      // Address to start iterating from is the last address in the stream buffer
      startAddr = streamBuffer->streamQueue.back().address;
   } else {
      // Address to start iterating from is the address that is being requested
      startAddr = addr;
      // Create temp address
      startAddr = startAddr >> this->blockoffset_size;
   }

   // Calculate how many blocks are in the stream buffer
   uint32_t blocksInStreamBuffer = streamBuffer->streamQueue.size();
   // Calculate how many blocks need to be added
   uint32_t blocksToAdd = this->prefetch_Unit->M - blocksInStreamBuffer;

   // Fill the stream buffer with new blocks starting from addr to addr + M - 1
   for (int i = 0; i < (int)blocksToAdd; i++) {

      // check if the stream buffer is over the max size
      if(this->prefetch_Unit->M <= (int)streamBuffer->streamQueue.size()){
         //std::cout << "Stream buffer is over the max size" << std::endl;
         break;
      }

      // Increment prefetches
      this->prefetches++;

      uint32_t newAddress = startAddr + i + 1;
      streamBuffer->streamQueue.push_back(newAddress);

      if(prefetch_from_lower_cache){
         // Send prefetch request to lower cache
         this->level_below->request(newAddress, 'r');
      } else {
         // Send prefetch request to memory
         this->memory_traffic++;
      }
   } 
   // Set valid bit
   streamBuffer->valid = true;

   // check all other stream buffers and decrement the ones that at greater than the current lruStreamCounter
   for(int i = 0; i < this->prefetch_Unit->N; i++){
      if(this->prefetch_Unit->streamBuffers[i].lruStreamBuffer > streamBuffer->lruStreamBuffer){
         this->prefetch_Unit->streamBuffers[i].lruStreamBuffer--;
      }
   }

   // set as most recently used
   streamBuffer->lruStreamBuffer = this->prefetch_Unit->N - 1;

   return;
}

void cache::printStreamBuffer(){
    std::cout << "===== Stream Buffer(s) contents =====" << std::endl;

    // Create a vector to hold the stream buffers in MRU to LRU order
    std::vector<std::vector<uint32_t> > orderedStreamBuffers(this->prefetch_Unit->N);

    // Map lruStreamBuffer values to indices (N-1 = MRU, 0 = LRU for lruStreamBuffer)
    for(int i = 0; i < this->prefetch_Unit->N; i++){
        // Find the stream buffer with lruStreamBuffer == i
        for(int j = 0; j < this->prefetch_Unit->N; j++){
            if(this->prefetch_Unit->streamBuffers[j].lruStreamBuffer == this->prefetch_Unit->N - i - 1){
                // If the stream buffer is valid
                if(this->prefetch_Unit->streamBuffers[j].valid){
                    // Copy the addresses into the ordered list
                    std::vector<uint32_t> buffer_contents;
                    for(const auto& block : this->prefetch_Unit->streamBuffers[j].streamQueue){
                        buffer_contents.push_back(block.address);
                    }
                    orderedStreamBuffers[i] = buffer_contents;
                } else {
                    // If invalid, store an empty vector
                    orderedStreamBuffers[i] = std::vector<uint32_t>();
                }
                break;
            }
        }
    }

    // Print the stream buffers from MRU to LRU
    for(int i = 0; i < this->prefetch_Unit->N; i++){
        // Print each address in the stream buffer
        for(const auto& addr : orderedStreamBuffers[i]){
            std::cout << std::hex << addr << " ";
        }
        std::cout << std::dec << std::endl;
    }
    std::cout << std::endl;
}


// Deal with parsed instruction/address
void cache::request(uint32_t addr, char rw){
   // Flags and indexes
   uint32_t LRUmax = this->cache_array[this->parse_index(addr)].LRU_max;
   uint32_t addr_tag = this->parse_tag(addr);

   if (rw == 'r'){ this->reads++; }
   else { this->writes++; }

   // Get pointer to the cache set the address indexes
   std::vector<cache_block> *setptr = this->cache_array[this->parse_index(addr)].getCacheSet();

   bool hit = false;
   int hit_index = 0;
   uint32_t LRU_index = 0;      // Tracks the highest LRU counter index relative to the current set
   uint32_t invalid_index = 0;  // Index of invalid block, if any blocks are invalid
   bool invalid = false;         // Tracks if any invalid blocks exist in the current set

   // Stream buffer logic
   bool stream_hit = false;
   uint32_t stream_hit_index = -1;
   int tempSizeOfPrefetchUnit = 1;

   // make sure prefetch unit is ena
   if(prefetch_enabled){
      tempSizeOfPrefetchUnit = this->prefetch_Unit->N;
   }

   // initialize order of use array
   streamBuffer* order_of_use[tempSizeOfPrefetchUnit];


   if(this->prefetch_enabled){
      // Initialize order of use array
      for(int i = 0; i < this->prefetch_Unit->N; i++){
         order_of_use[this->prefetch_Unit->streamBuffers[i].lruStreamBuffer] = &this->prefetch_Unit->streamBuffers[i];   // Set the index of the stream buffer in the order of use
      }

      // Check all stream buffers in order of least recently used to most recently used for a hit
      for(int i = this->prefetch_Unit->N - 1; i >= 0; i--){
         if(order_of_use[i]->valid) { 
            stream_hit = this->searchStreamBuffer(addr, order_of_use[i]); 
            if(stream_hit){
               stream_hit_index = i;
               break;
            }
         } 
      }

   } else {                // Invalid stream buffer or prefetch disabled
      stream_hit = false;
      stream_hit_index = -1;     // Set to -1 to indicate no stream buffer hit
   }


   // Loop through all blocks in indexed set and determine if the trace Hits
   for(int i = 0; i < (int)setptr->size(); i++){
      uint32_t set_tag = (*setptr)[i].tag;

      // Check LRU and Valid flags
      if ((*setptr)[i].valid == false && invalid == false){ invalid = true; invalid_index = i; }     // Find first invalid block that exists
      if ((*setptr)[i].LRUCounter == LRUmax){ LRU_index = i; }       // Find max LRU block
      
      // Check if block is in current set
      if (set_tag == addr_tag && (*setptr)[i].valid){
         // std::cout << this->cache_name << " - Cache HIT " << "\n" << std::endl;
         hit = true;
         hit_index = i;

         // If stream buffer hits
         if(stream_hit && this->prefetch_enabled){   // -- Scenario #4 Cache hit & Stream buffer hit
            // Update stream buffer
            this->updateStreamBuffer(addr, order_of_use[stream_hit_index]);   // Update the stream buffer that was hit
         } // -- Scenario #3 Cache hit & Stream buffer miss

         break;
      }
   }
   // HIT
   if(hit){
      // If write, set dirty bit
      if (rw == 'w'){
         (*setptr)[hit_index].dirty = true;
         //(*setptr)[i].data = addr;
      }
      (*setptr)[hit_index].valid = true;                   // Mark block as valid
      this->update_lru((*setptr)[hit_index].LRUCounter, addr, setptr, hit_index);      // Update LRU
   
   // CACHE MISS
   } else {
      // If stream buffer miss also    -- Scenario #1 Cache miss & Stream buffer miss
      if(!stream_hit && this->prefetch_enabled){
         // Initialize steam buffer with next address
         this->initializeStreamBuffer(addr, order_of_use[0]);     // Initialize the least recently used stream buffer
         // Increment prefetches
      } else if(stream_hit && this->prefetch_enabled){   // Scenario #2 Cache miss & Stream buffer hit
         // If stream buffer hit
         this->updateStreamBuffer(addr, order_of_use[stream_hit_index]);
      }

      // Get eviction index
      uint32_t replace_index = invalid ? invalid_index : LRU_index;

      // Writeback before read from higher level
      writeback_logic(addr, replace_index, setptr);

      if(rw == 'w'){ // Write MISS

         // check if stream buffer hit or missed
         if(!stream_hit){
            write_miss_count++;   // Only increment write miss count if stream buffer miss

            // Fetch block from next level (memory)
            if (this->level_below != NULL) { // If this is the last-level cache
               level_below->request(addr, 'r');
            } else {

               memory_traffic++;

            }
         }
         
         install_block(replace_index, addr, setptr, 'w');

      } else {    // Read MISS

         // Check if stream buffer hit or missed
         if(!stream_hit){
            read_miss_count++;   // Only increment read miss count if stream buffer miss
            // Fetch block from next level (memory)
            if (level_below != NULL) {
               this->level_below->request(addr, 'r');    // Read & Write requests both get send to lower level as read
            } else {

               memory_traffic++;
            }
         }

         install_block(replace_index, addr, setptr, 'r');
      }
   }
}
