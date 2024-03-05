#include "cache.h"

uint32_t CACHE::find_victim(uint32_t cpu, uint64_t instr_id, uint32_t set, const BLOCK* current_set, uint64_t ip, uint64_t full_addr, uint32_t type)
{
  // baseline LRU replacement policy for other caches
  return lru_victim(cpu, instr_id, set, current_set, ip, full_addr, type);
}

void CACHE::update_replacement_state(uint32_t cpu, uint32_t set, uint32_t way, uint64_t full_addr, uint64_t ip, uint64_t victim_addr, uint32_t type,
                                     uint8_t hit)
{
  if (type == WRITEBACK) {
    if (hit) // wrietback hit does not update LRU state
      return;
  }

  return lru_update(set, way);
}

// For L1 and L2 caches
uint32_t CACHE::lru_victim(uint32_t cpu, uint64_t instr_id, uint32_t set, const BLOCK* current_set, uint64_t ip, uint64_t full_addr, uint32_t type)
{
  uint32_t way = 0;

  // fill invalid line first
  for (way = 0; way < NUM_WAY; way++) {
    if (block[set][way].valid == false) {

      DP(if (warmup_complete[cpu]) {
        cout << "[" << NAME << "] " << __func__ << " instr_id: " << instr_id << " invalid set: " << set << " way: " << way;
        cout << hex << " address: " << (full_addr >> LOG2_BLOCK_SIZE) << " victim address: " << block[set][way].address << " data: " << block[set][way].data;
        cout << dec << " lru: " << block[set][way].lru << endl;
      });

      break;
    }
  }

  // LRU victim
  if (way == NUM_WAY) {
    for (way = 0; way < NUM_WAY; way++) {
      if (block[set][way].lru == NUM_WAY - 1) {

        DP(if (warmup_complete[cpu]) {
          cout << "[" << NAME << "] " << __func__ << " instr_id: " << instr_id << " replace set: " << set << " way: " << way;
          cout << hex << " address: " << (full_addr >> LOG2_BLOCK_SIZE) << " victim address: " << block[set][way].address << " data: " << block[set][way].data;
          cout << dec << " lru: " << block[set][way].lru << endl;
        });

        break;
      }
    }
  }

  if (way == NUM_WAY) {
    cerr << "[" << NAME << "] " << __func__ << " no victim! set: " << set << endl;
    assert(0);
  }

  return way;
}

// For Last-Level Cache
uint32_t CACHE::lru_victim_llc(uint32_t cpu, uint64_t instr_id, uint32_t set, const BLOCK* current_set, uint64_t ip, uint64_t full_addr, uint32_t type)
{
  uint32_t way = 0;

  // fill invalid line first
  for (way = 0; way < NUM_WAY; way++) {
    if (block[set][way].valid == 0)
      return way;
  }

  int count0 = 0; // Counter to count blocks of Core-0 in this LLC set
  int count1 = 0; // Counter to count blocks of Core-1 in this LLC set
  for (int i = 0; i < NUM_WAY; i++) {
    if (block[set][i].cpu == 0) {
      count0++; // number of block from core 0
    } else if (block[set][i].cpu == 1) {
      count1++; // number of blocks from core 1
    }
  }

  if (par > count0) // Case when number of blocks of core 0 is less than partition
  {
    if (cpu == 0) {
      uint32_t mx = 0;
      for (int i = 0; i < NUM_WAY; i++) {
        if (mx <= block[set][i].lru && block[set][i].cpu == 1) {
          mx = block[set][i].lru;
          way = i;
        }
      }
    } else {
      uint32_t mx = 0;
      for (int i = 0; i < NUM_WAY; i++) {

        if (mx <= block[set][i].lru && block[set][i].cpu == 1) {
          mx = block[set][i].lru;
          way = i;
        }
      }
    }
  }

  else if (par < count0) // Case when number of blocks of core 1 is less than partition
  {
    if (cpu == 0) {
      uint32_t mx = 0;
      for (uint32_t i = 0; i < NUM_WAY; i++) {
        if (mx <= block[set][i].lru && block[set][i].cpu == 0) {
          mx = block[set][i].lru;
          way = i;
        }
      }
    } else {
      uint32_t mx = 0;
      for (uint32_t i = 0; i < NUM_WAY; i++) {
        if (mx <= block[set][i].lru && block[set][i].cpu == 0) {
          mx = block[set][i].lru;
          way = i;
        }
      }
    }
  }

  else if (par == count0) // If partition is equal number of blocks of respective core
  {
    if (cpu == 0) {
      uint32_t mx = 0;
      for (uint32_t i = 0; i < NUM_WAY; i++) {

        if (mx <= block[set][i].lru && block[set][i].cpu == 0) // Finding the LRU way of core 0
        {
          mx = block[set][i].lru;
          way = i;
        }
      }
    } else {
      uint32_t mx = 0;
      for (int i = 0; i < NUM_WAY; i++) {
        if (mx <= block[set][i].lru && block[set][i].cpu == 1) // Finding the LRU way of core 1
        {
          mx = block[set][i].lru;
          way = i;
        }
      }
    }
  }

  if (way == NUM_WAY) {
    cerr << "[" << NAME << "] " << __func__ << " no victim! set: " << set << endl; // Error message in case no victim found
    assert(0);
  }

  return way;
}

void CACHE::lru_update(uint32_t set, uint32_t way) // Function to update L1 and L2 state after insertion
{
  // update lru replacement state
  for (uint32_t i = 0; i < NUM_WAY; i++) {
    if (block[set][i].lru < block[set][way].lru) {
      block[set][i].lru++;
    }
  }
  block[set][way].lru = 0; // promote to the MRU position
}

void CACHE::lru_update_llc(uint32_t set, uint32_t way, uint32_t cpu) // Function to update llc state after insertion
{
  // update lru replacement state
  for (uint32_t i = 0; i < NUM_WAY; i++) {
    if (block[set][i].cpu == cpu && block[set][i].lru < block[set][way].lru) {
      block[set][i].lru++;
    }
  }
  block[set][way].lru = 0; // promote to the MRU position
}

void CACHE::replacement_final_stats() {}

#ifdef NO_CRC2_COMPILE
void InitReplacementState() {}

uint32_t GetVictimInSet(uint32_t cpu, uint32_t set, const BLOCK* current_set, uint64_t PC, uint64_t paddr, uint32_t type) { return 0; }

void UpdateReplacementState(uint32_t cpu, uint32_t set, uint32_t way, uint64_t paddr, uint64_t PC, uint64_t victim_addr, uint32_t type, uint8_t hit) {}

void PrintStats_Heartbeat() {}

void PrintStats() {}
#endif