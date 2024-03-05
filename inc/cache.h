#ifndef CACHE_H
#define CACHE_H

#include <functional>
#include <list>
#include <string>
#include <vector>

#include "champsim.h"
#include "delay_queue.hpp"
#include "memory_class.h"
#include "ooo_cpu.h"
#include "operable.h"

// virtual address space prefetching
#define VA_PREFETCH_TRANSLATION_LATENCY 2

extern std::array<O3_CPU*, NUM_CPUS> ooo_cpu;

class CACHE : public champsim::operable, public MemoryRequestConsumer, public MemoryRequestProducer
{
public:
  uint32_t cpu;
  const std::string NAME;
  const uint32_t NUM_SET, NUM_WAY, WQ_SIZE, RQ_SIZE, PQ_SIZE, MSHR_SIZE;
  const uint32_t HIT_LATENCY, FILL_LATENCY, OFFSET_BITS;
  std::vector<BLOCK> block{NUM_SET * NUM_WAY};
  const uint32_t MAX_READ, MAX_WRITE;
  uint32_t reads_available_this_cycle, writes_available_this_cycle;
  const bool prefetch_as_load;
  const bool match_offset_bits;
  const bool virtual_prefetch;
  bool ever_seen_data = false;
  const unsigned pref_activate_mask = (1 << static_cast<int>(LOAD)) | (1 << static_cast<int>(PREFETCH));

  // prefetch stats
  uint64_t pf_requested = 0, pf_issued = 0, pf_useful = 0, pf_useless = 0, pf_fill = 0;

  // queues
  champsim::delay_queue<PACKET> RQ{RQ_SIZE, HIT_LATENCY}, // read queue
      PQ{PQ_SIZE, HIT_LATENCY},                           // prefetch queue
      VAPQ{PQ_SIZE, VA_PREFETCH_TRANSLATION_LATENCY},     // virtual address prefetch queue
      WQ{WQ_SIZE, HIT_LATENCY};                           // write queue

  std::list<PACKET> MSHR; // MSHR

  uint64_t sim_access[NUM_CPUS][NUM_TYPES] = {}, sim_hit[NUM_CPUS][NUM_TYPES] = {}, sim_miss[NUM_CPUS][NUM_TYPES] = {}, roi_access[NUM_CPUS][NUM_TYPES] = {},
           roi_hit[NUM_CPUS][NUM_TYPES] = {}, roi_miss[NUM_CPUS][NUM_TYPES] = {};

  uint64_t RQ_ACCESS = 0, RQ_MERGED = 0, RQ_FULL = 0, RQ_TO_CACHE = 0, PQ_ACCESS = 0, PQ_MERGED = 0, PQ_FULL = 0, PQ_TO_CACHE = 0, WQ_ACCESS = 0, WQ_MERGED = 0,
           WQ_FULL = 0, WQ_FORWARD = 0, WQ_TO_CACHE = 0;

  uint64_t total_miss_latency = 0;

<<<<<<< HEAD
  class CACHE : public MEMORY
  {
  public:
    uint32_t cpu;
    const string NAME;
    const uint32_t NUM_SET, NUM_WAY, NUM_LINE, WQ_SIZE, RQ_SIZE, PQ_SIZE, MSHR_SIZE;
    uint32_t LATENCY;
    BLOCK** block;
    int fill_level;
    uint32_t MAX_READ, MAX_FILL;
    uint32_t reads_available_this_cycle;
    uint8_t cache_type;
    uint32_t par;
=======
  // functions
  int add_rq(PACKET* packet) override;
  int add_wq(PACKET* packet) override;
  int add_pq(PACKET* packet) override;
>>>>>>> 29f568cfe2a6067b3a125d63742e37be0622d626

    void return_data(PACKET* packet) override;
    void operate() override;
    void operate_writes();
    void operate_reads();

    uint32_t get_occupancy(uint8_t queue_type, uint64_t address) override;
    uint32_t get_size(uint8_t queue_type, uint64_t address) override;

    uint32_t get_set(uint64_t address);
    uint32_t get_way(uint64_t address, uint32_t set);

    int invalidate_entry(uint64_t inval_addr);
    int prefetch_line(uint64_t pf_addr, bool fill_this_level, uint32_t prefetch_metadata);
    int prefetch_line(uint64_t ip, uint64_t base_addr, uint64_t pf_addr, bool fill_this_level, uint32_t prefetch_metadata); // deprecated

<<<<<<< HEAD
    LATENCY = 0;
    par = 8; // fixing initial partition to 8 and 8
=======
  void add_mshr(PACKET* packet);
  void va_translate_prefetches();
>>>>>>> 29f568cfe2a6067b3a125d63742e37be0622d626

    void handle_fill();
    void handle_writeback();
    void handle_read();
    void handle_prefetch();

    void readlike_hit(std::size_t set, std::size_t way, PACKET& handle_pkt);
    bool readlike_miss(PACKET& handle_pkt);
    bool filllike_miss(std::size_t set, std::size_t way, PACKET& handle_pkt);

    bool should_activate_prefetcher(int type);

    void print_deadlock() override;

#include "cache_modules.inc"

    const repl_t repl_type;
    const pref_t pref_type;

<<<<<<< HEAD
    pf_requested = 0;
    pf_issued = 0;
    pf_useful = 0;
    pf_useless = 0;
    pf_fill = 0;
  };

  // destructor
  ~CACHE()
  {
    for (uint32_t i = 0; i < NUM_SET; i++)
      delete[] block[i];
    delete[] block;
  };

  // functions
  int add_rq(PACKET*packet), add_wq(PACKET*packet), add_pq(PACKET*packet);

  void return_data(PACKET*packet), operate(), increment_WQ_FULL(uint64_t address);

  uint32_t get_occupancy(uint8_t queue_type, uint64_t address), get_size(uint8_t queue_type, uint64_t address);

  int check_hit(PACKET*packet), invalidate_entry(uint64_t inval_addr), check_mshr(PACKET*packet),
      prefetch_line(uint64_t ip, uint64_t base_addr, uint64_t pf_addr, int prefetch_fill_level, uint32_t prefetch_metadata),
      kpc_prefetch_line(uint64_t base_addr, uint64_t pf_addr, int prefetch_fill_level, int delta, int depth, int signature, int confidence,
                        uint32_t prefetch_metadata);

  void handle_fill(), handle_writeback(), handle_read(), handle_prefetch();

  void add_mshr(PACKET*packet), update_fill_cycle(), llc_initialize_replacement(),
      update_replacement_state(uint32_t cpu, uint32_t set, uint32_t way, uint64_t full_addr, uint64_t ip, uint64_t victim_addr, uint32_t type, uint8_t hit),
      llc_update_replacement_state(uint32_t cpu, uint32_t set, uint32_t way, uint64_t full_addr, uint64_t ip, uint64_t victim_addr, uint32_t type, uint8_t hit),
      lru_update(uint32_t set, uint32_t way), lru_update_llc(uint32_t set, uint32_t way, uint32_t cpu), fill_cache(uint32_t set, uint32_t way, PACKET*packet),
      replacement_final_stats(), llc_replacement_final_stats(),
      // prefetcher_initialize(),
      l1d_prefetcher_initialize(), l2c_prefetcher_initialize(), llc_prefetcher_initialize(),
      prefetcher_operate(uint64_t addr, uint64_t ip, uint8_t cache_hit, uint8_t type),
      l1d_prefetcher_operate(uint64_t addr, uint64_t ip, uint8_t cache_hit, uint8_t type),
      prefetcher_cache_fill(uint64_t addr, uint32_t set, uint32_t way, uint8_t prefetch, uint64_t evicted_addr),
      l1d_prefetcher_cache_fill(uint64_t addr, uint32_t set, uint32_t way, uint8_t prefetch, uint64_t evicted_addr, uint32_t metadata_in),
      // prefetcher_final_stats(),
      l1d_prefetcher_final_stats(), l2c_prefetcher_final_stats(), llc_prefetcher_final_stats();
  void (*l1i_prefetcher_cache_operate)(uint32_t, uint64_t, uint8_t, uint8_t);
  void (*l1i_prefetcher_cache_fill)(uint32_t, uint64_t, uint32_t, uint32_t, uint8_t, uint64_t);

  uint32_t l2c_prefetcher_operate(uint64_t addr, uint64_t ip, uint8_t cache_hit, uint8_t type, uint32_t metadata_in),
      llc_prefetcher_operate(uint64_t addr, uint64_t ip, uint8_t cache_hit, uint8_t type, uint32_t metadata_in),
      l2c_prefetcher_cache_fill(uint64_t addr, uint32_t set, uint32_t way, uint8_t prefetch, uint64_t evicted_addr, uint32_t metadata_in),
      llc_prefetcher_cache_fill(uint64_t addr, uint32_t set, uint32_t way, uint8_t prefetch, uint64_t evicted_addr, uint32_t metadata_in);

  uint32_t get_set(uint64_t address), get_way(uint64_t address, uint32_t set),
      find_victim(uint32_t cpu, uint64_t instr_id, uint32_t set, const BLOCK*current_set, uint64_t ip, uint64_t full_addr, uint32_t type),
      llc_find_victim(uint32_t cpu, uint64_t instr_id, uint32_t set, const BLOCK*current_set, uint64_t ip, uint64_t full_addr, uint32_t type),
      lru_victim(uint32_t cpu, uint64_t instr_id, uint32_t set, const BLOCK*current_set, uint64_t ip, uint64_t full_addr, uint32_t type),
      lru_victim_llc(uint32_t cpu, uint64_t instr_id, uint32_t set, const BLOCK*current_set, uint64_t ip, uint64_t full_addr, uint32_t type);
=======
  // constructor
  CACHE(std::string v1, double freq_scale, unsigned fill_level, uint32_t v2, int v3, uint32_t v5, uint32_t v6, uint32_t v7, uint32_t v8, uint32_t hit_lat,
        uint32_t fill_lat, uint32_t max_read, uint32_t max_write, std::size_t offset_bits, bool pref_load, bool wq_full_addr, bool va_pref,
        unsigned pref_act_mask, MemoryRequestConsumer* ll, pref_t pref, repl_t repl)
      : champsim::operable(freq_scale), MemoryRequestConsumer(fill_level), MemoryRequestProducer(ll), NAME(v1), NUM_SET(v2), NUM_WAY(v3), WQ_SIZE(v5),
        RQ_SIZE(v6), PQ_SIZE(v7), MSHR_SIZE(v8), HIT_LATENCY(hit_lat), FILL_LATENCY(fill_lat), OFFSET_BITS(offset_bits), MAX_READ(max_read),
        MAX_WRITE(max_write), prefetch_as_load(pref_load), match_offset_bits(wq_full_addr), virtual_prefetch(va_pref), pref_activate_mask(pref_act_mask),
        repl_type(repl), pref_type(pref)
  {
  }
>>>>>>> 29f568cfe2a6067b3a125d63742e37be0622d626
};

#endif
