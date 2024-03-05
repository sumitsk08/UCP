#include "cache.h"

<<<<<<< HEAD
class ATD
{
public:
  uint64_t tag;
  uint32_t lru;

  ATD()
  { // Constructor for ATD
    tag = 0;
    lru = 0;
  };
};

ATD arr0[32][LLC_WAY]; // ATD for core 1
ATD arr1[32][LLC_WAY]; // ATD for core 2

int util_core0[LLC_WAY]; // Array of counters to count number of hits at each LRU position in core 0
int util_core1[LLC_WAY]; // Array of counters to count number of hits at each LRU position in core 1

void miss_llc(const PACKET* packet, uint32_t set, uint32_t cpu)
{
  if (cpu == 0) {
    int mx_lru = -1;
    int w = -1;
    for (int i = 0; i < LLC_WAY; i++) {
      if (mx_lru < arr0[set][i].lru) { // finding the lru position in ATD to eliminate ans storing the corresponding way
        mx_lru = arr0[set][i].lru;
        w = i;
      }
    }
    if (mx_lru < 15) { // If the maximum age is less than 15 than the set is empty and we incrementing the age of each block in the set
      arr0[set][w].lru = 0;
      arr0[set][w].tag = packet->address;
      for (int j = 0; j < LLC_WAY; j++) {
        if (arr0[set][j].lru < mx_lru) {
          arr0[set][j].lru++;
        }
      }
      return;
    }
    for (int i = 0; i < LLC_WAY;
         i++) { // If the maximum age equal to 15 than the set is full we are then incrementing the age of each block in the set by 1 and inserting in MRU
      if (arr0[set][i].lru == LLC_WAY - 1) {
        arr0[set][i].lru = 0;
        arr0[set][i].tag = packet->address;
        for (int j = 0; j < LLC_WAY; j++) {
          if (arr0[set][j].lru < LLC_WAY - 1) {
            arr0[set][j].lru++;
          }
        }
      }
    }
  } else if (cpu == 1) {
    int mx_lru = -1;
    int w = -1;
    for (int i = 0; i < LLC_WAY; i++) {
      if (mx_lru < arr0[set][i].lru) { // finding the lru position in ATD to eliminate ans storing the corresponding way
        mx_lru = arr0[set][i].lru;
        w = i;
      }
    }
    if (mx_lru < 15) { // If the maximum age is less than 15 than the set is empty and we incrementing the age of each block in the set
      arr1[set][w].lru = 0;
      arr1[set][w].tag = packet->address;
      for (int j = 0; j < LLC_WAY; j++) {
        if (arr1[set][j].lru < mx_lru) {
          arr1[set][j].lru++;
        }
      }
      return;
    }
    for (int i = 0; i < LLC_WAY;
         i++) { // If the maximum age equal to 15 than the set is full we are then incrementing the age of each block in the set by 1 and inserting in MRU
      if (arr1[set][i].lru == LLC_WAY - 1) {
        arr1[set][i].lru = 0;
        arr1[set][i].tag = packet->address;
        for (int j = 0; j < LLC_WAY; j++) {
          if (arr1[set][j].lru < LLC_WAY - 1) {
            arr1[set][j].lru++;
          }
        }
      }
    }
  }
}

void atd_llc(uint32_t set, const PACKET* packet, uint32_t cpu)
{
  if (set >= 32) { // Sampled 32 sets
    return;
  }
  if (cpu == 0) {
    int flag = 0;
    for (int i = 0; i < LLC_WAY; i++) {
      if (packet->address == arr0[set][i].tag) { // Checking for hit
        util_core0[arr0[set][i].lru]++;
        arr0[set][i].lru = 0; // Promoting to MRU in case of hit
        arr0[set][i].tag = packet->address;

        for (int j = 0; j < LLC_WAY; j++) {
          if (arr0[set][j].lru < arr0[set][i].lru) {
            arr0[set][j].lru++; // Incrementing LRU value of other ways
          }
        }
        flag = 1;
        break;
      }
    }
    if (flag == 0) {
      miss_llc(packet, set, cpu); // Calling miss_llc in case of miss in LLC
    }
  } else if (cpu == 1) {
    int flag = 0;
    for (int i = 0; i < LLC_WAY; i++) {
      if (packet->address == arr1[set][i].tag) { // Checking for hit
        util_core1[arr1[set][i].lru]++;
        arr1[set][i].lru = 0; // Promoting to MRU in case of hit
        arr1[set][i].tag = packet->address;
        for (int j = 0; j < LLC_WAY; j++) {
          if (arr1[set][j].lru < arr1[set][i].lru) {
            arr1[set][j].lru++; // Incrementing LRU value of other ways
          }
        }
        flag = 1;
        break;
      }
    }
    if (flag == 0) {
      miss_llc(packet, set, cpu); // Calling miss_llc in case of miss in LLC
    }
  }
}

int ucp_par()
{ // Function to find best partition
  int mx = -1;
  int x;
  for (int i = 0; i <= 14; i++) {
    if (util_core0[i] + util_core1[LLC_WAY - 2 - i] > mx) {
      x = i;
      mx = util_core0[i] + util_core1[LLC_WAY - 2 - i];
    }
  }
  return x; // Returning best partition
}

uint64_t l2pf_access = 0;
=======
#include <algorithm>
#include <iterator>

#include "champsim.h"
#include "champsim_constants.h"
#include "util.h"
#include "vmem.h"

#ifndef SANITY_CHECK
#define NDEBUG
#endif

extern VirtualMemory vmem;
extern uint8_t warmup_complete[NUM_CPUS];
>>>>>>> 29f568cfe2a6067b3a125d63742e37be0622d626

void CACHE::handle_fill()
{
  while (writes_available_this_cycle > 0) {
    auto fill_mshr = MSHR.begin();
    if (fill_mshr == std::end(MSHR) || fill_mshr->event_cycle > current_cycle)
      return;

    // find victim
    uint32_t set = get_set(fill_mshr->address);

    auto set_begin = std::next(std::begin(block), set * NUM_WAY);
    auto set_end = std::next(set_begin, NUM_WAY);
    auto first_inv = std::find_if_not(set_begin, set_end, is_valid<BLOCK>());
    uint32_t way = std::distance(set_begin, first_inv);
    if (way == NUM_WAY)
      way = impl_replacement_find_victim(fill_mshr->cpu, fill_mshr->instr_id, set, &block.data()[set * NUM_WAY], fill_mshr->ip, fill_mshr->address,
                                         fill_mshr->type);

    bool success = filllike_miss(set, way, *fill_mshr);
    if (!success)
      return;

    if (way != NUM_WAY) {
      // update processed packets
      fill_mshr->data = block[set * NUM_WAY + way].data;

      for (auto ret : fill_mshr->to_return)
        ret->return_data(&(*fill_mshr));
    }

    MSHR.erase(fill_mshr);
    writes_available_this_cycle--;
  }
}

void CACHE::handle_writeback()
{
  while (writes_available_this_cycle > 0) {
    if (!WQ.has_ready())
      return;

    // handle the oldest entry
    PACKET& handle_pkt = WQ.front();

<<<<<<< HEAD
    // access cache
    uint32_t set = get_set(WQ.entry[index].address);
    int way = check_hit(&WQ.entry[index]);
    if (cache_type == IS_LLC) {
      atd_llc(set, &WQ.entry[index], writeback_cpu);
    }

    if (way >= 0) { // writeback hit (or RFO hit for L1D)
=======
    // access cache
    uint32_t set = get_set(handle_pkt.address);
    uint32_t way = get_way(handle_pkt.address, set);
>>>>>>> 29f568cfe2a6067b3a125d63742e37be0622d626

      BLOCK& fill_block = block[set * NUM_WAY + way];

      if (way < NUM_WAY) // HIT
      {
        impl_replacement_update_state(handle_pkt.cpu, set, way, fill_block.address, handle_pkt.ip, 0, handle_pkt.type, 1);

        // COLLECT STATS
        sim_hit[handle_pkt.cpu][handle_pkt.type]++;
        sim_access[handle_pkt.cpu][handle_pkt.type]++;

        // mark dirty
        fill_block.dirty = 1;
      } else // MISS
      {
        bool success;
        if (handle_pkt.type == RFO && handle_pkt.to_return.empty()) {
          success = readlike_miss(handle_pkt);
        } else {
          // find victim
          auto set_begin = std::next(std::begin(block), set * NUM_WAY);
          auto set_end = std::next(set_begin, NUM_WAY);
          auto first_inv = std::find_if_not(set_begin, set_end, is_valid<BLOCK>());
          way = std::distance(set_begin, first_inv);
          if (way == NUM_WAY)
            way = impl_replacement_find_victim(handle_pkt.cpu, handle_pkt.instr_id, set, &block.data()[set * NUM_WAY], handle_pkt.ip, handle_pkt.address,
                                               handle_pkt.type);

          success = filllike_miss(set, way, handle_pkt);
        }

        if (!success)
          return;
      }

      // remove this entry from WQ
      writes_available_this_cycle--;
      WQ.pop_front();
    }
  }

  void CACHE::handle_read()
  {
    while (reads_available_this_cycle > 0) {

      if (!RQ.has_ready())
        return;

      // handle the oldest entry
      PACKET& handle_pkt = RQ.front();

      // A (hopefully temporary) hack to know whether to send the evicted paddr or
      // vaddr to the prefetcher
      ever_seen_data |= (handle_pkt.v_address != handle_pkt.ip);

      uint32_t set = get_set(handle_pkt.address);
      uint32_t way = get_way(handle_pkt.address, set);

      if (way < NUM_WAY) // HIT
      {
        readlike_hit(set, way, handle_pkt);
      } else {
        bool success = readlike_miss(handle_pkt);
        if (!success)
          return;
<<<<<<< HEAD

        // handle the oldest entry
        if ((RQ.entry[RQ.head].event_cycle <= current_core_cycle[read_cpu]) && (RQ.occupancy > 0)) {
          int index = RQ.head;

          // access cache
          uint32_t set = get_set(RQ.entry[index].address);
          int way = check_hit(&RQ.entry[index]);
          if (cache_type == IS_LLC) {
            atd_llc(set, &RQ.entry[index], read_cpu);
          }

          if (way >= 0) { // read hit

            if (cache_type == IS_ITLB) {
              RQ.entry[index].instruction_pa = block[set][way].data;
              if (PROCESSED.occupancy < PROCESSED.SIZE)
                PROCESSED.add_queue(&RQ.entry[index]);
            } else if (cache_type == IS_DTLB) {
              RQ.entry[index].data_pa = block[set][way].data;
              if (PROCESSED.occupancy < PROCESSED.SIZE)
                PROCESSED.add_queue(&RQ.entry[index]);
            } else if (cache_type == IS_STLB)
              RQ.entry[index].data = block[set][way].data;
            else if (cache_type == IS_L1I) {
              if (PROCESSED.occupancy < PROCESSED.SIZE)
                PROCESSED.add_queue(&RQ.entry[index]);
            }
            // else if (cache_type == IS_L1D) {
            else if ((cache_type == IS_L1D) && (RQ.entry[index].type != PREFETCH)) {
              if (PROCESSED.occupancy < PROCESSED.SIZE)
                PROCESSED.add_queue(&RQ.entry[index]);
            }

            // update prefetcher on load instruction
            if (RQ.entry[index].type == LOAD) {
              if (cache_type == IS_L1I)
                l1i_prefetcher_cache_operate(read_cpu, RQ.entry[index].ip, 1, block[set][way].prefetch);
              if (cache_type == IS_L1D)
                l1d_prefetcher_operate(RQ.entry[index].full_addr, RQ.entry[index].ip, 1, RQ.entry[index].type);
              else if (cache_type == IS_L2C)
                l2c_prefetcher_operate(block[set][way].address << LOG2_BLOCK_SIZE, RQ.entry[index].ip, 1, RQ.entry[index].type, 0);
              else if (cache_type == IS_LLC) {
                cpu = read_cpu;
                llc_prefetcher_operate(block[set][way].address << LOG2_BLOCK_SIZE, RQ.entry[index].ip, 1, RQ.entry[index].type, 0);
                cpu = 0;
              }
            }

            // update replacement policy
            if (cache_type == IS_LLC) {
              llc_update_replacement_state(read_cpu, set, way, block[set][way].full_addr, RQ.entry[index].ip, 0, RQ.entry[index].type, 1);

            } else
              update_replacement_state(read_cpu, set, way, block[set][way].full_addr, RQ.entry[index].ip, 0, RQ.entry[index].type, 1);

            // COLLECT STATS
            sim_hit[read_cpu][RQ.entry[index].type]++;
            sim_access[read_cpu][RQ.entry[index].type]++;

            // check fill level
            if (RQ.entry[index].fill_level < fill_level) {

              if (fill_level == FILL_L2) {
                if (RQ.entry[index].fill_l1i) {
                  upper_level_icache[read_cpu]->return_data(&RQ.entry[index]);
                }
                if (RQ.entry[index].fill_l1d) {
                  upper_level_dcache[read_cpu]->return_data(&RQ.entry[index]);
                }
              } else {
                if (RQ.entry[index].instruction)
                  upper_level_icache[read_cpu]->return_data(&RQ.entry[index]);
                if (RQ.entry[index].is_data)
                  upper_level_dcache[read_cpu]->return_data(&RQ.entry[index]);
              }
            }

            // update prefetch stats and reset prefetch bit
            if (block[set][way].prefetch) {
              pf_useful++;
              block[set][way].prefetch = 0;
            }
            block[set][way].used = 1;

            HIT[RQ.entry[index].type]++;
            ACCESS[RQ.entry[index].type]++;

            // remove this entry from RQ
            RQ.remove_queue(&RQ.entry[index]);
            reads_available_this_cycle--;
          } else { // read miss

            DP(if (warmup_complete[read_cpu]) {
              cout << "[" << NAME << "] " << __func__ << " read miss";
              cout << " instr_id: " << RQ.entry[index].instr_id << " address: " << hex << RQ.entry[index].address;
              cout << " full_addr: " << RQ.entry[index].full_addr << dec;
              cout << " cycle: " << RQ.entry[index].event_cycle << endl;
            });

            // check mshr
            uint8_t miss_handled = 1;
            int mshr_index = check_mshr(&RQ.entry[index]);

            if (mshr_index == -2) {
              // this is a data/instruction collision in the MSHR, so we have to wait before we can allocate this miss
              miss_handled = 0;
            } else if ((mshr_index == -1) && (MSHR.occupancy < MSHR_SIZE)) { // this is a new miss

              if (cache_type == IS_LLC) {
                // check to make sure the DRAM RQ has room for this LLC read miss
                if (lower_level->get_occupancy(1, RQ.entry[index].address) == lower_level->get_size(1, RQ.entry[index].address)) {
                  miss_handled = 0;
                } else {
                  add_mshr(&RQ.entry[index]);
                  if (lower_level) {
                    lower_level->add_rq(&RQ.entry[index]);
                  }
                }
              } else {
                // add it to mshr (read miss)
                add_mshr(&RQ.entry[index]);

                // add it to the next level's read queue
                if (lower_level)
                  lower_level->add_rq(&RQ.entry[index]);
                else { // this is the last level
                  if (cache_type == IS_STLB) {
                    // TODO: need to differentiate page table walk and actual swap

                    // emulate page table walk
                    uint64_t pa = va_to_pa(read_cpu, RQ.entry[index].instr_id, RQ.entry[index].full_addr, RQ.entry[index].address, 0);

                    RQ.entry[index].data = pa >> LOG2_PAGE_SIZE;
                    RQ.entry[index].event_cycle = current_core_cycle[read_cpu];
                    return_data(&RQ.entry[index]);
                  }
                }
              }
            } else {
              if ((mshr_index == -1) && (MSHR.occupancy == MSHR_SIZE)) { // not enough MSHR resource

                // cannot handle miss request until one of MSHRs is available
                miss_handled = 0;
                STALL[RQ.entry[index].type]++;
              } else if (mshr_index != -1) { // already in-flight miss

                // mark merged consumer
                if (RQ.entry[index].type == RFO) {

                  if (RQ.entry[index].tlb_access) {
                    uint32_t sq_index = RQ.entry[index].sq_index;
                    MSHR.entry[mshr_index].store_merged = 1;
                    MSHR.entry[mshr_index].sq_index_depend_on_me.insert(sq_index);
                    MSHR.entry[mshr_index].sq_index_depend_on_me.join(RQ.entry[index].sq_index_depend_on_me, SQ_SIZE);
                  }

                  if (RQ.entry[index].load_merged) {
                    // uint32_t lq_index = RQ.entry[index].lq_index;
                    MSHR.entry[mshr_index].load_merged = 1;
                    // MSHR.entry[mshr_index].lq_index_depend_on_me[lq_index] = 1;
                    MSHR.entry[mshr_index].lq_index_depend_on_me.join(RQ.entry[index].lq_index_depend_on_me, LQ_SIZE);
                  }
                } else {
                  if (RQ.entry[index].instruction) {
                    uint32_t rob_index = RQ.entry[index].rob_index;
                    MSHR.entry[mshr_index].instruction = 1; // add as instruction type
                    MSHR.entry[mshr_index].instr_merged = 1;
                    MSHR.entry[mshr_index].rob_index_depend_on_me.insert(rob_index);

                    DP(if (warmup_complete[MSHR.entry[mshr_index].cpu]) {
                      cout << "[INSTR_MERGED] " << __func__ << " cpu: " << MSHR.entry[mshr_index].cpu << " instr_id: " << MSHR.entry[mshr_index].instr_id;
                      cout << " merged rob_index: " << rob_index << " instr_id: " << RQ.entry[index].instr_id << endl;
                    });

                    if (RQ.entry[index].instr_merged) {
                      MSHR.entry[mshr_index].rob_index_depend_on_me.join(RQ.entry[index].rob_index_depend_on_me, ROB_SIZE);
                      DP(if (warmup_complete[MSHR.entry[mshr_index].cpu]) {
                        cout << "[INSTR_MERGED] " << __func__ << " cpu: " << MSHR.entry[mshr_index].cpu << " instr_id: " << MSHR.entry[mshr_index].instr_id;
                        cout << " merged rob_index: " << i << " instr_id: N/A" << endl;
                      });
                    }
                  } else {
                    uint32_t lq_index = RQ.entry[index].lq_index;
                    MSHR.entry[mshr_index].is_data = 1; // add as data type
                    MSHR.entry[mshr_index].load_merged = 1;
                    MSHR.entry[mshr_index].lq_index_depend_on_me.insert(lq_index);

                    DP(if (warmup_complete[read_cpu]) {
                      cout << "[DATA_MERGED] " << __func__ << " cpu: " << read_cpu << " instr_id: " << RQ.entry[index].instr_id;
                      cout << " merged rob_index: " << RQ.entry[index].rob_index << " instr_id: " << RQ.entry[index].instr_id
                           << " lq_index: " << RQ.entry[index].lq_index << endl;
                    });
                    MSHR.entry[mshr_index].lq_index_depend_on_me.join(RQ.entry[index].lq_index_depend_on_me, LQ_SIZE);
                    if (RQ.entry[index].store_merged) {
                      MSHR.entry[mshr_index].store_merged = 1;
                      MSHR.entry[mshr_index].sq_index_depend_on_me.join(RQ.entry[index].sq_index_depend_on_me, SQ_SIZE);
                    }
                  }
                }

                // update fill_level
                if (RQ.entry[index].fill_level < MSHR.entry[mshr_index].fill_level)
                  MSHR.entry[mshr_index].fill_level = RQ.entry[index].fill_level;

                if ((RQ.entry[index].fill_l1i) && (MSHR.entry[mshr_index].fill_l1i != 1)) {
                  MSHR.entry[mshr_index].fill_l1i = 1;
                }
                if ((RQ.entry[index].fill_l1d) && (MSHR.entry[mshr_index].fill_l1d != 1)) {
                  MSHR.entry[mshr_index].fill_l1d = 1;
                }

                // update request
                if (MSHR.entry[mshr_index].type == PREFETCH) {
                  uint8_t prior_returned = MSHR.entry[mshr_index].returned;
                  uint64_t prior_event_cycle = MSHR.entry[mshr_index].event_cycle;
                  MSHR.entry[mshr_index] = RQ.entry[index];

                  // in case request is already returned, we should keep event_cycle and retunred variables
                  MSHR.entry[mshr_index].returned = prior_returned;
                  MSHR.entry[mshr_index].event_cycle = prior_event_cycle;
                }

                MSHR_MERGED[RQ.entry[index].type]++;

                DP(if (warmup_complete[read_cpu]) {
                  cout << "[" << NAME << "] " << __func__ << " mshr merged";
                  cout << " instr_id: " << RQ.entry[index].instr_id << " prior_id: " << MSHR.entry[mshr_index].instr_id;
                  cout << " address: " << hex << RQ.entry[index].address;
                  cout << " full_addr: " << RQ.entry[index].full_addr << dec;
                  cout << " cycle: " << RQ.entry[index].event_cycle << endl;
                });
              } else { // WE SHOULD NOT REACH HERE
                cerr << "[" << NAME << "] MSHR errors" << endl;
                assert(0);
              }
            }

            if (miss_handled) {
              // update prefetcher on load instruction
              if (RQ.entry[index].type == LOAD) {
                if (cache_type == IS_L1I)
                  l1i_prefetcher_cache_operate(read_cpu, RQ.entry[index].ip, 0, 0);
                if (cache_type == IS_L1D)
                  l1d_prefetcher_operate(RQ.entry[index].full_addr, RQ.entry[index].ip, 0, RQ.entry[index].type);
                if (cache_type == IS_L2C)
                  l2c_prefetcher_operate(RQ.entry[index].address << LOG2_BLOCK_SIZE, RQ.entry[index].ip, 0, RQ.entry[index].type, 0);
                if (cache_type == IS_LLC) {
                  cpu = read_cpu;
                  llc_prefetcher_operate(RQ.entry[index].address << LOG2_BLOCK_SIZE, RQ.entry[index].ip, 0, RQ.entry[index].type, 0);
                  cpu = 0;
                }
              }

              MISS[RQ.entry[index].type]++;
              ACCESS[RQ.entry[index].type]++;

              // remove this entry from RQ
              RQ.remove_queue(&RQ.entry[index]);
              reads_available_this_cycle--;
            }
          }
        } else {
          return;
        }

        if (reads_available_this_cycle == 0) {
          return;
        }
=======
>>>>>>> 29f568cfe2a6067b3a125d63742e37be0622d626
      }

      // remove this entry from RQ
      RQ.pop_front();
      reads_available_this_cycle--;
    }
  }

  void CACHE::handle_prefetch()
  {
    while (reads_available_this_cycle > 0) {
      if (!PQ.has_ready())
        return;

      // handle the oldest entry
      PACKET& handle_pkt = PQ.front();

      uint32_t set = get_set(handle_pkt.address);
      uint32_t way = get_way(handle_pkt.address, set);

      if (way < NUM_WAY) // HIT
      {
        readlike_hit(set, way, handle_pkt);
      } else {
        bool success = readlike_miss(handle_pkt);
        if (!success)
          return;
      }

      // remove this entry from PQ
      PQ.pop_front();
      reads_available_this_cycle--;
    }
  }

  void CACHE::readlike_hit(std::size_t set, std::size_t way, PACKET & handle_pkt)
  {
    DP(if (warmup_complete[handle_pkt.cpu]) {
      std::cout << "[" << NAME << "] " << __func__ << " hit";
      std::cout << " instr_id: " << handle_pkt.instr_id << " address: " << std::hex << (handle_pkt.address >> OFFSET_BITS);
      std::cout << " full_addr: " << handle_pkt.address;
      std::cout << " full_v_addr: " << handle_pkt.v_address << std::dec;
      std::cout << " type: " << +handle_pkt.type;
      std::cout << " cycle: " << current_cycle << std::endl;
    });

    BLOCK& hit_block = block[set * NUM_WAY + way];

    handle_pkt.data = hit_block.data;

    // update prefetcher on load instruction
    if (should_activate_prefetcher(handle_pkt.type) && handle_pkt.pf_origin_level < fill_level) {
      cpu = handle_pkt.cpu;
      uint64_t pf_base_addr = (virtual_prefetch ? handle_pkt.v_address : handle_pkt.address) & ~bitmask(match_offset_bits ? 0 : OFFSET_BITS);
      handle_pkt.pf_metadata = impl_prefetcher_cache_operate(pf_base_addr, handle_pkt.ip, 1, handle_pkt.type, handle_pkt.pf_metadata);
    }

    // update replacement policy
    impl_replacement_update_state(handle_pkt.cpu, set, way, hit_block.address, handle_pkt.ip, 0, handle_pkt.type, 1);

    // COLLECT STATS
    sim_hit[handle_pkt.cpu][handle_pkt.type]++;
    sim_access[handle_pkt.cpu][handle_pkt.type]++;

    for (auto ret : handle_pkt.to_return)
      ret->return_data(&handle_pkt);

    // update prefetch stats and reset prefetch bit
    if (hit_block.prefetch) {
      pf_useful++;
      hit_block.prefetch = 0;
    }
  }

  bool CACHE::readlike_miss(PACKET & handle_pkt)
  {
    DP(if (warmup_complete[handle_pkt.cpu]) {
      std::cout << "[" << NAME << "] " << __func__ << " miss";
      std::cout << " instr_id: " << handle_pkt.instr_id << " address: " << std::hex << (handle_pkt.address >> OFFSET_BITS);
      std::cout << " full_addr: " << handle_pkt.address;
      std::cout << " full_v_addr: " << handle_pkt.v_address << std::dec;
      std::cout << " type: " << +handle_pkt.type;
      std::cout << " cycle: " << current_cycle << std::endl;
    });

    // check mshr
    auto mshr_entry = std::find_if(MSHR.begin(), MSHR.end(), eq_addr<PACKET>(handle_pkt.address, OFFSET_BITS));
    bool mshr_full = (MSHR.size() == MSHR_SIZE);

    if (mshr_entry != MSHR.end()) // miss already inflight
    {
      // update fill location
      mshr_entry->fill_level = std::min(mshr_entry->fill_level, handle_pkt.fill_level);

      packet_dep_merge(mshr_entry->lq_index_depend_on_me, handle_pkt.lq_index_depend_on_me);
      packet_dep_merge(mshr_entry->sq_index_depend_on_me, handle_pkt.sq_index_depend_on_me);
      packet_dep_merge(mshr_entry->instr_depend_on_me, handle_pkt.instr_depend_on_me);
      packet_dep_merge(mshr_entry->to_return, handle_pkt.to_return);

      if (mshr_entry->type == PREFETCH && handle_pkt.type != PREFETCH) {
        // Mark the prefetch as useful
        if (mshr_entry->pf_origin_level == fill_level)
          pf_useful++;

        uint64_t prior_event_cycle = mshr_entry->event_cycle;
        *mshr_entry = handle_pkt;

        // in case request is already returned, we should keep event_cycle
        mshr_entry->event_cycle = prior_event_cycle;
      }
    } else {
      if (mshr_full)  // not enough MSHR resource
        return false; // TODO should we allow prefetches anyway if they will not
                      // be filled to this level?

      bool is_read = prefetch_as_load || (handle_pkt.type != PREFETCH);

      // check to make sure the lower level queue has room for this read miss
      int queue_type = (is_read) ? 1 : 3;
      if (lower_level->get_occupancy(queue_type, handle_pkt.address) == lower_level->get_size(queue_type, handle_pkt.address))
        return false;

      // Allocate an MSHR
      if (handle_pkt.fill_level <= fill_level) {
        auto it = MSHR.insert(std::end(MSHR), handle_pkt);
        it->cycle_enqueued = current_cycle;
        it->event_cycle = std::numeric_limits<uint64_t>::max();
      }

      if (handle_pkt.fill_level <= fill_level)
        handle_pkt.to_return = {this};
      else
        handle_pkt.to_return.clear();

      if (!is_read)
        lower_level->add_pq(&handle_pkt);
      else
        lower_level->add_rq(&handle_pkt);
    }

    // update prefetcher on load instructions and prefetches from upper levels
    if (should_activate_prefetcher(handle_pkt.type) && handle_pkt.pf_origin_level < fill_level) {
      cpu = handle_pkt.cpu;
      uint64_t pf_base_addr = (virtual_prefetch ? handle_pkt.v_address : handle_pkt.address) & ~bitmask(match_offset_bits ? 0 : OFFSET_BITS);
      handle_pkt.pf_metadata = impl_prefetcher_cache_operate(pf_base_addr, handle_pkt.ip, 0, handle_pkt.type, handle_pkt.pf_metadata);
    }

    return true;
  }

  bool CACHE::filllike_miss(std::size_t set, std::size_t way, PACKET & handle_pkt)
  {
    DP(if (warmup_complete[handle_pkt.cpu]) {
      std::cout << "[" << NAME << "] " << __func__ << " miss";
      std::cout << " instr_id: " << handle_pkt.instr_id << " address: " << std::hex << (handle_pkt.address >> OFFSET_BITS);
      std::cout << " full_addr: " << handle_pkt.address;
      std::cout << " full_v_addr: " << handle_pkt.v_address << std::dec;
      std::cout << " type: " << +handle_pkt.type;
      std::cout << " cycle: " << current_cycle << std::endl;
    });

    bool bypass = (way == NUM_WAY);
#ifndef LLC_BYPASS
    assert(!bypass);
#endif
    assert(handle_pkt.type != WRITEBACK || !bypass);

    BLOCK& fill_block = block[set * NUM_WAY + way];
    bool evicting_dirty = !bypass && (lower_level != NULL) && fill_block.dirty;
    uint64_t evicting_address = 0;

    if (!bypass) {
      if (evicting_dirty) {
        PACKET writeback_packet;

        writeback_packet.fill_level = lower_level->fill_level;
        writeback_packet.cpu = handle_pkt.cpu;
        writeback_packet.address = fill_block.address;
        writeback_packet.data = fill_block.data;
        writeback_packet.instr_id = handle_pkt.instr_id;
        writeback_packet.ip = 0;
        writeback_packet.type = WRITEBACK;

        auto result = lower_level->add_wq(&writeback_packet);
        if (result == -2)
          return false;
      }

      if (ever_seen_data)
        evicting_address = fill_block.address & ~bitmask(match_offset_bits ? 0 : OFFSET_BITS);
      else
        evicting_address = fill_block.v_address & ~bitmask(match_offset_bits ? 0 : OFFSET_BITS);

      if (fill_block.prefetch)
        pf_useless++;

      if (handle_pkt.type == PREFETCH)
        pf_fill++;

      fill_block.valid = true;
      fill_block.prefetch = (handle_pkt.type == PREFETCH && handle_pkt.pf_origin_level == fill_level);
      fill_block.dirty = (handle_pkt.type == WRITEBACK || (handle_pkt.type == RFO && handle_pkt.to_return.empty()));
      fill_block.address = handle_pkt.address;
      fill_block.v_address = handle_pkt.v_address;
      fill_block.data = handle_pkt.data;
      fill_block.ip = handle_pkt.ip;
      fill_block.cpu = handle_pkt.cpu;
      fill_block.instr_id = handle_pkt.instr_id;
    }

    if (warmup_complete[handle_pkt.cpu] && (handle_pkt.cycle_enqueued != 0))
      total_miss_latency += current_cycle - handle_pkt.cycle_enqueued;

    // update prefetcher
    cpu = handle_pkt.cpu;
    handle_pkt.pf_metadata =
        impl_prefetcher_cache_fill((virtual_prefetch ? handle_pkt.v_address : handle_pkt.address) & ~bitmask(match_offset_bits ? 0 : OFFSET_BITS), set, way,
                                   handle_pkt.type == PREFETCH, evicting_address, handle_pkt.pf_metadata);

    // update replacement policy
    impl_replacement_update_state(handle_pkt.cpu, set, way, handle_pkt.address, handle_pkt.ip, 0, handle_pkt.type, 0);

    // COLLECT STATS
    sim_miss[handle_pkt.cpu][handle_pkt.type]++;
    sim_access[handle_pkt.cpu][handle_pkt.type]++;

    return true;
  }

  void CACHE::operate()
  {
    operate_writes();
    operate_reads();

    impl_prefetcher_cycle_operate();
  }

  void CACHE::operate_writes()
  {
    // perform all writes
    writes_available_this_cycle = MAX_WRITE;
    handle_fill();
    handle_writeback();

    WQ.operate();
  }

  void CACHE::operate_reads()
  {
    // perform all reads
    reads_available_this_cycle = MAX_READ;
    handle_read();
    va_translate_prefetches();
    handle_prefetch();

    RQ.operate();
    PQ.operate();
    VAPQ.operate();
  }

  uint32_t CACHE::get_set(uint64_t address) { return ((address >> OFFSET_BITS) & bitmask(lg2(NUM_SET))); }

  uint32_t CACHE::get_way(uint64_t address, uint32_t set)
  {
<<<<<<< HEAD
    for (uint32_t way = 0; way < NUM_WAY; way++) {
      if (block[set][way].valid && (block[set][way].tag == address))
        return way;
    }

    return NUM_WAY;
  }

  void CACHE::fill_cache(uint32_t set, uint32_t way, PACKET * packet)
  {
#ifdef SANITY_CHECK
    if (cache_type == IS_ITLB) {
      if (packet->data == 0)
        assert(0);
    }

    if (cache_type == IS_DTLB) {
      if (packet->data == 0)
        assert(0);
    }

    if (cache_type == IS_STLB) {
      if (packet->data == 0)
        assert(0);
    }
#endif
    if (block[set][way].prefetch && (block[set][way].used == 0))
      pf_useless++;

    if (block[set][way].valid == 0)
      block[set][way].valid = 1;
    block[set][way].dirty = 0;
    block[set][way].prefetch = (packet->type == PREFETCH) ? 1 : 0;
    block[set][way].used = 0;

    if (block[set][way].prefetch)
      pf_fill++;

    block[set][way].delta = packet->delta;
    block[set][way].depth = packet->depth;
    block[set][way].signature = packet->signature;
    block[set][way].confidence = packet->confidence;

    block[set][way].tag = packet->address;
    block[set][way].address = packet->address;
    block[set][way].full_addr = packet->full_addr;
    block[set][way].data = packet->data;
    block[set][way].ip = packet->ip;
    block[set][way].cpu = packet->cpu;
    block[set][way].instr_id = packet->instr_id;

    DP(if (warmup_complete[packet->cpu]) {
      cout << "[" << NAME << "] " << __func__ << " set: " << set << " way: " << way;
      cout << " lru: " << block[set][way].lru << " tag: " << hex << block[set][way].tag << " full_addr: " << block[set][way].full_addr;
      cout << " data: " << block[set][way].data << dec << endl;
    });
  }

  int CACHE::check_hit(PACKET * packet)
  {
    uint32_t set = get_set(packet->address);
    int match_way = -1;

    atd_llc(set, packet, packet->cpu);

    if (NUM_SET < set) {
      cerr << "[" << NAME << "_ERROR] " << __func__ << " invalid set index: " << set << " NUM_SET: " << NUM_SET;
      cerr << " address: " << hex << packet->address << " full_addr: " << packet->full_addr << dec;
      cerr << " event: " << packet->event_cycle << endl;
      assert(0);
    }

    // hit
    for (uint32_t way = 0; way < NUM_WAY; way++) {
      if (block[set][way].valid && (block[set][way].tag == packet->address)) {

        match_way = way;

        DP(if (warmup_complete[packet->cpu]) {
          cout << "[" << NAME << "] " << __func__ << " instr_id: " << packet->instr_id << " type: " << +packet->type << hex << " addr: " << packet->address;
          cout << " full_addr: " << packet->full_addr << " tag: " << block[set][way].tag << " data: " << block[set][way].data << dec;
          cout << " set: " << set << " way: " << way << " lru: " << block[set][way].lru;
          cout << " event: " << packet->event_cycle << " cycle: " << current_core_cycle[cpu] << endl;
        });

        break;
      }
    }

    return match_way;
=======
  auto begin = std::next(block.begin(), set * NUM_WAY);
  auto end = std::next(begin, NUM_WAY);
  return std::distance(begin, std::find_if(begin, end, eq_addr<BLOCK>(address, OFFSET_BITS)));
>>>>>>> 29f568cfe2a6067b3a125d63742e37be0622d626
  }

  int CACHE::invalidate_entry(uint64_t inval_addr)
  {
    uint32_t set = get_set(inval_addr);
    uint32_t way = get_way(inval_addr, set);

    if (way < NUM_WAY)
      block[set * NUM_WAY + way].valid = 0;

    return way;
  }

  int CACHE::add_rq(PACKET * packet)
  {
    assert(packet->address != 0);
    RQ_ACCESS++;

    DP(if (warmup_complete[packet->cpu]) {
      std::cout << "[" << NAME << "_RQ] " << __func__ << " instr_id: " << packet->instr_id << " address: " << std::hex << (packet->address >> OFFSET_BITS);
      std::cout << " full_addr: " << packet->address << " v_address: " << packet->v_address << std::dec << " type: " << +packet->type
                << " occupancy: " << RQ.occupancy();
    })

    // check for the latest writebacks in the write queue
    champsim::delay_queue<PACKET>::iterator found_wq =
        std::find_if(WQ.begin(), WQ.end(), eq_addr<PACKET>(packet->address, match_offset_bits ? 0 : OFFSET_BITS));

    if (found_wq != WQ.end()) {

      DP(if (warmup_complete[packet->cpu]) std::cout << " MERGED_WQ" << std::endl;)

      packet->data = found_wq->data;
      for (auto ret : packet->to_return)
        ret->return_data(packet);

      WQ_FORWARD++;
      return -1;
    }

    // check for duplicates in the read queue
    auto found_rq = std::find_if(RQ.begin(), RQ.end(), eq_addr<PACKET>(packet->address, OFFSET_BITS));
    if (found_rq != RQ.end()) {

      DP(if (warmup_complete[packet->cpu]) std::cout << " MERGED_RQ" << std::endl;)

      packet_dep_merge(found_rq->lq_index_depend_on_me, packet->lq_index_depend_on_me);
      packet_dep_merge(found_rq->sq_index_depend_on_me, packet->sq_index_depend_on_me);
      packet_dep_merge(found_rq->instr_depend_on_me, packet->instr_depend_on_me);
      packet_dep_merge(found_rq->to_return, packet->to_return);

      RQ_MERGED++;

      return 0; // merged index
    }

    // check occupancy
    if (RQ.full()) {
      RQ_FULL++;

      DP(if (warmup_complete[packet->cpu]) std::cout << " FULL" << std::endl;)

      return -2; // cannot handle this request
    }

    // if there is no duplicate, add it to RQ
    if (warmup_complete[cpu])
      RQ.push_back(*packet);
    else
      RQ.push_back_ready(*packet);

    DP(if (warmup_complete[packet->cpu]) std::cout << " ADDED" << std::endl;)

    RQ_TO_CACHE++;
    return RQ.occupancy();
  }

  int CACHE::add_wq(PACKET * packet)
  {
    WQ_ACCESS++;

    DP(if (warmup_complete[packet->cpu]) {
      std::cout << "[" << NAME << "_WQ] " << __func__ << " instr_id: " << packet->instr_id << " address: " << std::hex << (packet->address >> OFFSET_BITS);
      std::cout << " full_addr: " << packet->address << " v_address: " << packet->v_address << std::dec << " type: " << +packet->type
                << " occupancy: " << RQ.occupancy();
    })

    // check for duplicates in the write queue
    champsim::delay_queue<PACKET>::iterator found_wq =
        std::find_if(WQ.begin(), WQ.end(), eq_addr<PACKET>(packet->address, match_offset_bits ? 0 : OFFSET_BITS));

    if (found_wq != WQ.end()) {

      DP(if (warmup_complete[packet->cpu]) std::cout << " MERGED" << std::endl;)

      WQ_MERGED++;
      return 0; // merged index
    }

    // Check for room in the queue
    if (WQ.full()) {
      DP(if (warmup_complete[packet->cpu]) std::cout << " FULL" << std::endl;)

      ++WQ_FULL;
      return -2;
    }

    // if there is no duplicate, add it to the write queue
    if (warmup_complete[cpu])
      WQ.push_back(*packet);
    else
      WQ.push_back_ready(*packet);

    DP(if (warmup_complete[packet->cpu]) std::cout << " ADDED" << std::endl;)

    WQ_TO_CACHE++;
    WQ_ACCESS++;

    return WQ.occupancy();
  }

  int CACHE::prefetch_line(uint64_t pf_addr, bool fill_this_level, uint32_t prefetch_metadata)
  {
    pf_requested++;

    PACKET pf_packet;
    pf_packet.type = PREFETCH;
    pf_packet.fill_level = (fill_this_level ? fill_level : lower_level->fill_level);
    pf_packet.pf_origin_level = fill_level;
    pf_packet.pf_metadata = prefetch_metadata;
    pf_packet.cpu = cpu;
    pf_packet.address = pf_addr;
    pf_packet.v_address = virtual_prefetch ? pf_addr : 0;

    if (virtual_prefetch) {
      if (!VAPQ.full()) {
        VAPQ.push_back(pf_packet);
        return 1;
      }
    } else {
      int result = add_pq(&pf_packet);
      if (result != -2) {
        if (result > 0)
          pf_issued++;
        return 1;
      }
    }

    return 0;
  }

  int CACHE::prefetch_line(uint64_t ip, uint64_t base_addr, uint64_t pf_addr, bool fill_this_level, uint32_t prefetch_metadata)
  {
    static bool deprecate_printed = false;
    if (!deprecate_printed) {
      std::cout << "WARNING: The extended signature CACHE::prefetch_line(ip, "
                   "base_addr, pf_addr, fill_this_level, prefetch_metadata) is "
                   "deprecated."
                << std::endl;
      std::cout << "WARNING: Use CACHE::prefetch_line(pf_addr, fill_this_level, "
                   "prefetch_metadata) instead."
                << std::endl;
      deprecate_printed = true;
    }
    return prefetch_line(pf_addr, fill_this_level, prefetch_metadata);
  }

  void CACHE::va_translate_prefetches()
  {
    // TEMPORARY SOLUTION: mark prefetches as translated after a fixed latency
    if (VAPQ.has_ready()) {
      VAPQ.front().address = vmem.va_to_pa(cpu, VAPQ.front().v_address).first;

      // move the translated prefetch over to the regular PQ
      int result = add_pq(&VAPQ.front());

      // remove the prefetch from the VAPQ
      if (result != -2)
        VAPQ.pop_front();

      if (result > 0)
        pf_issued++;
    }
  }

  int CACHE::add_pq(PACKET * packet)
  {
    assert(packet->address != 0);
    PQ_ACCESS++;

    DP(if (warmup_complete[packet->cpu]) {
      std::cout << "[" << NAME << "_WQ] " << __func__ << " instr_id: " << packet->instr_id << " address: " << std::hex << (packet->address >> OFFSET_BITS);
      std::cout << " full_addr: " << packet->address << " v_address: " << packet->v_address << std::dec << " type: " << +packet->type
                << " occupancy: " << RQ.occupancy();
    })

    // check for the latest wirtebacks in the write queue
    champsim::delay_queue<PACKET>::iterator found_wq =
        std::find_if(WQ.begin(), WQ.end(), eq_addr<PACKET>(packet->address, match_offset_bits ? 0 : OFFSET_BITS));

    if (found_wq != WQ.end()) {

      DP(if (warmup_complete[packet->cpu]) std::cout << " MERGED_WQ" << std::endl;)

      packet->data = found_wq->data;
      for (auto ret : packet->to_return)
        ret->return_data(packet);

      WQ_FORWARD++;
      return -1;
    }

    // check for duplicates in the PQ
    auto found = std::find_if(PQ.begin(), PQ.end(), eq_addr<PACKET>(packet->address, OFFSET_BITS));
    if (found != PQ.end()) {
      DP(if (warmup_complete[packet->cpu]) std::cout << " MERGED_PQ" << std::endl;)

      found->fill_level = std::min(found->fill_level, packet->fill_level);
      packet_dep_merge(found->to_return, packet->to_return);

      PQ_MERGED++;
      return 0;
    }

    // check occupancy
    if (PQ.full()) {

      DP(if (warmup_complete[packet->cpu]) std::cout << " FULL" << std::endl;)

      PQ_FULL++;
      return -2; // cannot handle this request
    }

    // if there is no duplicate, add it to PQ
    if (warmup_complete[cpu])
      PQ.push_back(*packet);
    else
      PQ.push_back_ready(*packet);

    DP(if (warmup_complete[packet->cpu]) std::cout << " ADDED" << std::endl;)

    PQ_TO_CACHE++;
    return PQ.occupancy();
  }

  void CACHE::return_data(PACKET * packet)
  {
    // check MSHR information
    auto mshr_entry = std::find_if(MSHR.begin(), MSHR.end(), eq_addr<PACKET>(packet->address, OFFSET_BITS));
    auto first_unreturned = std::find_if(MSHR.begin(), MSHR.end(), [](auto x) { return x.event_cycle == std::numeric_limits<uint64_t>::max(); });

    // sanity check
    if (mshr_entry == MSHR.end()) {
      std::cerr << "[" << NAME << "_MSHR] " << __func__ << " instr_id: " << packet->instr_id << " cannot find a matching entry!";
      std::cerr << " address: " << std::hex << packet->address;
      std::cerr << " v_address: " << packet->v_address;
      std::cerr << " address: " << (packet->address >> OFFSET_BITS) << std::dec;
      std::cerr << " event: " << packet->event_cycle << " current: " << current_cycle << std::endl;
      assert(0);
    }

    // MSHR holds the most updated information about this request
    mshr_entry->data = packet->data;
    mshr_entry->pf_metadata = packet->pf_metadata;
    mshr_entry->event_cycle = current_cycle + (warmup_complete[cpu] ? FILL_LATENCY : 0);

    DP(if (warmup_complete[packet->cpu]) {
      std::cout << "[" << NAME << "_MSHR] " << __func__ << " instr_id: " << mshr_entry->instr_id;
      std::cout << " address: " << std::hex << (mshr_entry->address >> OFFSET_BITS) << " full_addr: " << mshr_entry->address;
      std::cout << " data: " << mshr_entry->data << std::dec;
      std::cout << " index: " << std::distance(MSHR.begin(), mshr_entry) << " occupancy: " << get_occupancy(0, 0);
      std::cout << " event: " << mshr_entry->event_cycle << " current: " << current_cycle << std::endl;
    });

    // Order this entry after previously-returned entries, but before non-returned
    // entries
    std::iter_swap(mshr_entry, first_unreturned);
  }

  uint32_t CACHE::get_occupancy(uint8_t queue_type, uint64_t address)
  {
    if (queue_type == 0)
      return std::count_if(MSHR.begin(), MSHR.end(), is_valid<PACKET>());
    else if (queue_type == 1)
      return RQ.occupancy();
    else if (queue_type == 2)
      return WQ.occupancy();
    else if (queue_type == 3)
      return PQ.occupancy();

    return 0;
  }

  uint32_t CACHE::get_size(uint8_t queue_type, uint64_t address)
  {
    if (queue_type == 0)
      return MSHR_SIZE;
    else if (queue_type == 1)
      return RQ.size();
    else if (queue_type == 2)
      return WQ.size();
    else if (queue_type == 3)
      return PQ.size();

    return 0;
  }

  bool CACHE::should_activate_prefetcher(int type) { return (1 << static_cast<int>(type)) & pref_activate_mask; }

  void CACHE::print_deadlock()
  {
    if (!std::empty(MSHR)) {
      std::cout << NAME << " MSHR Entry" << std::endl;
      std::size_t j = 0;
      for (PACKET entry : MSHR) {
        std::cout << "[" << NAME << " MSHR] entry: " << j++ << " instr_id: " << entry.instr_id;
        std::cout << " address: " << std::hex << (entry.address >> LOG2_BLOCK_SIZE) << " full_addr: " << entry.address << std::dec << " type: " << +entry.type;
        std::cout << " fill_level: " << +entry.fill_level << " event_cycle: " << entry.event_cycle << std::endl;
      }
    } else {
      std::cout << NAME << " MSHR empty" << std::endl;
    }
  }
