#include <algorithm>
#include <array>
#include <fstream>
<<<<<<< HEAD
#include "cache.h"

    int ucp_par();

=======
#include <functional>
#include <getopt.h>
#include <iomanip>
#include <signal.h>
#include <string.h>
#include <vector>
>>>>>>> 29f568cfe2a6067b3a125d63742e37be0622d626

#include "cache.h"
#include "champsim.h"
#include "champsim_constants.h"
#include "dram_controller.h"
#include "ooo_cpu.h"
#include "operable.h"
#include "tracereader.h"
#include "vmem.h"

    uint8_t warmup_complete[NUM_CPUS] = {}, simulation_complete[NUM_CPUS] = {}, all_warmup_complete = 0, all_simulation_complete = 0, MAX_INSTR_DESTINATIONS = NUM_INSTR_DESTINATIONS, knob_cloudsuite = 0, knob_low_bandwidth = 0;

uint64_t warmup_instructions = 1000000, simulation_instructions = 10000000;

auto start_time = time(NULL);

// For backwards compatibility with older module source.
champsim::deprecated_clock_cycle current_core_cycle;

extern MEMORY_CONTROLLER DRAM;
extern VirtualMemory vmem;
extern std::array<O3_CPU*, NUM_CPUS> ooo_cpu;
extern std::array<CACHE*, NUM_CACHES> caches;
extern std::array<champsim::operable*, NUM_OPERABLES> operables;

std::vector<tracereader*> traces;

uint64_t champsim::deprecated_clock_cycle::operator[](std::size_t cpu_idx)
{
  static bool deprecate_printed = false;
  if (!deprecate_printed) {
    std::cout << "WARNING: The use of 'current_core_cycle[cpu]' is deprecated." << std::endl;
    std::cout << "WARNING: Use 'this->current_cycle' instead." << std::endl;
    deprecate_printed = true;
  }
  return ooo_cpu[cpu_idx]->current_cycle;
}

void record_roi_stats(uint32_t cpu, CACHE* cache)
{
  for (uint32_t i = 0; i < NUM_TYPES; i++) {
    cache->roi_access[cpu][i] = cache->sim_access[cpu][i];
    cache->roi_hit[cpu][i] = cache->sim_hit[cpu][i];
    cache->roi_miss[cpu][i] = cache->sim_miss[cpu][i];
  }
}

void print_roi_stats(uint32_t cpu, CACHE* cache)
{
  uint64_t TOTAL_ACCESS = 0, TOTAL_HIT = 0, TOTAL_MISS = 0;

  for (uint32_t i = 0; i < NUM_TYPES; i++) {
    TOTAL_ACCESS += cache->roi_access[cpu][i];
    TOTAL_HIT += cache->roi_hit[cpu][i];
    TOTAL_MISS += cache->roi_miss[cpu][i];
  }

  if (TOTAL_ACCESS > 0) {
    cout << cache->NAME;
    cout << " TOTAL     ACCESS: " << setw(10) << TOTAL_ACCESS << "  HIT: " << setw(10) << TOTAL_HIT << "  MISS: " << setw(10) << TOTAL_MISS << endl;

    cout << cache->NAME;
    cout << " LOAD      ACCESS: " << setw(10) << cache->roi_access[cpu][0] << "  HIT: " << setw(10) << cache->roi_hit[cpu][0] << "  MISS: " << setw(10)
         << cache->roi_miss[cpu][0] << endl;

    cout << cache->NAME;
    cout << " RFO       ACCESS: " << setw(10) << cache->roi_access[cpu][1] << "  HIT: " << setw(10) << cache->roi_hit[cpu][1] << "  MISS: " << setw(10)
         << cache->roi_miss[cpu][1] << endl;

    cout << cache->NAME;
    cout << " PREFETCH  ACCESS: " << setw(10) << cache->roi_access[cpu][2] << "  HIT: " << setw(10) << cache->roi_hit[cpu][2] << "  MISS: " << setw(10)
         << cache->roi_miss[cpu][2] << endl;

    cout << cache->NAME;
    cout << " WRITEBACK ACCESS: " << setw(10) << cache->roi_access[cpu][3] << "  HIT: " << setw(10) << cache->roi_hit[cpu][3] << "  MISS: " << setw(10)
         << cache->roi_miss[cpu][3] << endl;

    cout << cache->NAME;
    cout << " TRANSLATION ACCESS: " << setw(10) << cache->roi_access[cpu][4] << "  HIT: " << setw(10) << cache->roi_hit[cpu][4] << "  MISS: " << setw(10)
         << cache->roi_miss[cpu][4] << endl;

    cout << cache->NAME;
    cout << " PREFETCH  REQUESTED: " << setw(10) << cache->pf_requested << "  ISSUED: " << setw(10) << cache->pf_issued;
    cout << "  USEFUL: " << setw(10) << cache->pf_useful << "  USELESS: " << setw(10) << cache->pf_useless << endl;

    cout << cache->NAME;
    cout << " AVERAGE MISS LATENCY: " << (1.0 * (cache->total_miss_latency)) / TOTAL_MISS << " cycles" << endl;
    // cout << " AVERAGE MISS LATENCY: " <<
    // (cache->total_miss_latency)/TOTAL_MISS << " cycles " <<
    // cache->total_miss_latency << "/" << TOTAL_MISS<< endl;
  }
}

void print_sim_stats(uint32_t cpu, CACHE* cache)
{
  uint64_t TOTAL_ACCESS = 0, TOTAL_HIT = 0, TOTAL_MISS = 0;

  for (uint32_t i = 0; i < NUM_TYPES; i++) {
    TOTAL_ACCESS += cache->sim_access[cpu][i];
    TOTAL_HIT += cache->sim_hit[cpu][i];
    TOTAL_MISS += cache->sim_miss[cpu][i];
  }

  if (TOTAL_ACCESS > 0) {
    cout << cache->NAME;
    cout << " TOTAL     ACCESS: " << setw(10) << TOTAL_ACCESS << "  HIT: " << setw(10) << TOTAL_HIT << "  MISS: " << setw(10) << TOTAL_MISS << endl;

    cout << cache->NAME;
    cout << " LOAD      ACCESS: " << setw(10) << cache->sim_access[cpu][0] << "  HIT: " << setw(10) << cache->sim_hit[cpu][0] << "  MISS: " << setw(10)
         << cache->sim_miss[cpu][0] << endl;

    cout << cache->NAME;
    cout << " RFO       ACCESS: " << setw(10) << cache->sim_access[cpu][1] << "  HIT: " << setw(10) << cache->sim_hit[cpu][1] << "  MISS: " << setw(10)
         << cache->sim_miss[cpu][1] << endl;

    cout << cache->NAME;
    cout << " PREFETCH  ACCESS: " << setw(10) << cache->sim_access[cpu][2] << "  HIT: " << setw(10) << cache->sim_hit[cpu][2] << "  MISS: " << setw(10)
         << cache->sim_miss[cpu][2] << endl;

    cout << cache->NAME;
    cout << " WRITEBACK ACCESS: " << setw(10) << cache->sim_access[cpu][3] << "  HIT: " << setw(10) << cache->sim_hit[cpu][3] << "  MISS: " << setw(10)
         << cache->sim_miss[cpu][3] << endl;
  }
}

void print_branch_stats()
{
  for (uint32_t i = 0; i < NUM_CPUS; i++) {
    cout << endl << "CPU " << i << " Branch Prediction Accuracy: ";
    cout << (100.0 * (ooo_cpu[i]->num_branch - ooo_cpu[i]->branch_mispredictions)) / ooo_cpu[i]->num_branch;
    cout << "% MPKI: " << (1000.0 * ooo_cpu[i]->branch_mispredictions) / (ooo_cpu[i]->num_retired - warmup_instructions);
    cout << " Average ROB Occupancy at Mispredict: " << (1.0 * ooo_cpu[i]->total_rob_occupancy_at_branch_mispredict) / ooo_cpu[i]->branch_mispredictions
         << endl;

    /*
    cout << "Branch types" << endl;
    cout << "NOT_BRANCH: " << ooo_cpu[i]->total_branch_types[0] << " " <<
    (100.0*ooo_cpu[i]->total_branch_types[0])/(ooo_cpu[i]->num_retired -
    ooo_cpu[i]->begin_sim_instr) << "%" << endl; cout << "BRANCH_DIRECT_JUMP: "
    << ooo_cpu[i]->total_branch_types[1] << " " <<
    (100.0*ooo_cpu[i]->total_branch_types[1])/(ooo_cpu[i]->num_retired -
    ooo_cpu[i]->begin_sim_instr) << "%" << endl; cout << "BRANCH_INDIRECT: " <<
    ooo_cpu[i]->total_branch_types[2] << " " <<
    (100.0*ooo_cpu[i]->total_branch_types[2])/(ooo_cpu[i]->num_retired -
    ooo_cpu[i]->begin_sim_instr) << "%" << endl; cout << "BRANCH_CONDITIONAL: "
    << ooo_cpu[i]->total_branch_types[3] << " " <<
    (100.0*ooo_cpu[i]->total_branch_types[3])/(ooo_cpu[i]->num_retired -
    ooo_cpu[i]->begin_sim_instr) << "%" << endl; cout << "BRANCH_DIRECT_CALL: "
    << ooo_cpu[i]->total_branch_types[4] << " " <<
    (100.0*ooo_cpu[i]->total_branch_types[4])/(ooo_cpu[i]->num_retired -
    ooo_cpu[i]->begin_sim_instr) << "%" << endl; cout << "BRANCH_INDIRECT_CALL:
    " << ooo_cpu[i]->total_branch_types[5] << " " <<
    (100.0*ooo_cpu[i]->total_branch_types[5])/(ooo_cpu[i]->num_retired -
    ooo_cpu[i]->begin_sim_instr) << "%" << endl; cout << "BRANCH_RETURN: " <<
    ooo_cpu[i]->total_branch_types[6] << " " <<
    (100.0*ooo_cpu[i]->total_branch_types[6])/(ooo_cpu[i]->num_retired -
    ooo_cpu[i]->begin_sim_instr) << "%" << endl; cout << "BRANCH_OTHER: " <<
    ooo_cpu[i]->total_branch_types[7] << " " <<
    (100.0*ooo_cpu[i]->total_branch_types[7])/(ooo_cpu[i]->num_retired -
    ooo_cpu[i]->begin_sim_instr) << "%" << endl << endl;
    */

    cout << "Branch type MPKI" << endl;
    cout << "BRANCH_DIRECT_JUMP: " << (1000.0 * ooo_cpu[i]->branch_type_misses[1] / (ooo_cpu[i]->num_retired - ooo_cpu[i]->begin_sim_instr)) << endl;
    cout << "BRANCH_INDIRECT: " << (1000.0 * ooo_cpu[i]->branch_type_misses[2] / (ooo_cpu[i]->num_retired - ooo_cpu[i]->begin_sim_instr)) << endl;
    cout << "BRANCH_CONDITIONAL: " << (1000.0 * ooo_cpu[i]->branch_type_misses[3] / (ooo_cpu[i]->num_retired - ooo_cpu[i]->begin_sim_instr)) << endl;
    cout << "BRANCH_DIRECT_CALL: " << (1000.0 * ooo_cpu[i]->branch_type_misses[4] / (ooo_cpu[i]->num_retired - ooo_cpu[i]->begin_sim_instr)) << endl;
    cout << "BRANCH_INDIRECT_CALL: " << (1000.0 * ooo_cpu[i]->branch_type_misses[5] / (ooo_cpu[i]->num_retired - ooo_cpu[i]->begin_sim_instr)) << endl;
    cout << "BRANCH_RETURN: " << (1000.0 * ooo_cpu[i]->branch_type_misses[6] / (ooo_cpu[i]->num_retired - ooo_cpu[i]->begin_sim_instr)) << endl << endl;
  }
}

void print_dram_stats()
{
  uint64_t total_congested_cycle = 0;
  uint64_t total_congested_count = 0;

  std::cout << std::endl;
  std::cout << "DRAM Statistics" << std::endl;
  for (uint32_t i = 0; i < DRAM_CHANNELS; i++) {
    std::cout << " CHANNEL " << i << std::endl;

    auto& channel = DRAM.channels[i];
    std::cout << " RQ ROW_BUFFER_HIT: " << std::setw(10) << channel.RQ_ROW_BUFFER_HIT << " ";
    std::cout << " ROW_BUFFER_MISS: " << std::setw(10) << channel.RQ_ROW_BUFFER_MISS;
    std::cout << std::endl;

    std::cout << " DBUS AVG_CONGESTED_CYCLE: ";
    if (channel.dbus_count_congested)
      std::cout << std::setw(10) << ((double)channel.dbus_cycle_congested / channel.dbus_count_congested);
    else
      std::cout << "-";
    std::cout << std::endl;

    std::cout << " WQ ROW_BUFFER_HIT: " << std::setw(10) << channel.WQ_ROW_BUFFER_HIT << " ";
    std::cout << " ROW_BUFFER_MISS: " << std::setw(10) << channel.WQ_ROW_BUFFER_MISS << " ";
    std::cout << " FULL: " << std::setw(10) << channel.WQ_FULL;
    std::cout << std::endl;

    std::cout << std::endl;

    total_congested_cycle += channel.dbus_cycle_congested;
    total_congested_count += channel.dbus_count_congested;
  }

  if (DRAM_CHANNELS > 1) {
    std::cout << " DBUS AVG_CONGESTED_CYCLE: ";
    if (total_congested_count)
      std::cout << std::setw(10) << ((double)total_congested_cycle / total_congested_count);
    else
      std::cout << "-";

    std::cout << std::endl;
  }
}

void reset_cache_stats(uint32_t cpu, CACHE* cache)
{
  for (uint32_t i = 0; i < NUM_TYPES; i++) {
    cache->sim_access[cpu][i] = 0;
    cache->sim_hit[cpu][i] = 0;
    cache->sim_miss[cpu][i] = 0;
  }

  cache->pf_requested = 0;
  cache->pf_issued = 0;
  cache->pf_useful = 0;
  cache->pf_useless = 0;
  cache->pf_fill = 0;

  cache->total_miss_latency = 0;

  cache->RQ_ACCESS = 0;
  cache->RQ_MERGED = 0;
  cache->RQ_TO_CACHE = 0;

  cache->WQ_ACCESS = 0;
  cache->WQ_MERGED = 0;
  cache->WQ_TO_CACHE = 0;
  cache->WQ_FORWARD = 0;
  cache->WQ_FULL = 0;
}

void finish_warmup()
{
  uint64_t elapsed_second = (uint64_t)(time(NULL) - start_time), elapsed_minute = elapsed_second / 60, elapsed_hour = elapsed_minute / 60;
  elapsed_minute -= elapsed_hour * 60;
  elapsed_second -= (elapsed_hour * 3600 + elapsed_minute * 60);

  // reset core latency
  // note: since re-ordering he function calls in the main simulation loop, it's
  // no longer necessary to add
  //       extra latency for scheduling and execution, unless you want these
  //       steps to take longer than 1 cycle.
  // PAGE_TABLE_LATENCY = 100;
  // SWAP_LATENCY = 100000;

  cout << endl;
  for (uint32_t i = 0; i < NUM_CPUS; i++) {
    cout << "Warmup complete CPU " << i << " instructions: " << ooo_cpu[i]->num_retired << " cycles: " << ooo_cpu[i]->current_cycle;
    cout << " (Simulation time: " << elapsed_hour << " hr " << elapsed_minute << " min " << elapsed_second << " sec) " << endl;

    ooo_cpu[i]->begin_sim_cycle = ooo_cpu[i]->current_cycle;
    ooo_cpu[i]->begin_sim_instr = ooo_cpu[i]->num_retired;

    // reset branch stats
    ooo_cpu[i]->num_branch = 0;
    ooo_cpu[i]->branch_mispredictions = 0;
    ooo_cpu[i]->total_rob_occupancy_at_branch_mispredict = 0;

    for (uint32_t j = 0; j < 8; j++) {
      ooo_cpu[i]->total_branch_types[j] = 0;
      ooo_cpu[i]->branch_type_misses[j] = 0;
    }

    for (auto it = caches.rbegin(); it != caches.rend(); ++it)
      reset_cache_stats(i, *it);
  }
  cout << endl;

  // reset DRAM stats
  for (uint32_t i = 0; i < DRAM_CHANNELS; i++) {
    DRAM.channels[i].WQ_ROW_BUFFER_HIT = 0;
    DRAM.channels[i].WQ_ROW_BUFFER_MISS = 0;
    DRAM.channels[i].RQ_ROW_BUFFER_HIT = 0;
    DRAM.channels[i].RQ_ROW_BUFFER_MISS = 0;
  }
}

void signal_handler(int signal)
{
  cout << "Caught signal: " << signal << endl;
  exit(1);
}

int main(int argc, char** argv)
{
  // interrupt signal hanlder
  struct sigaction sigIntHandler;
  sigIntHandler.sa_handler = signal_handler;
  sigemptyset(&sigIntHandler.sa_mask);
  sigIntHandler.sa_flags = 0;
  sigaction(SIGINT, &sigIntHandler, NULL);

  cout << endl << "*** ChampSim Multicore Out-of-Order Simulator ***" << endl << endl;

  // initialize knobs
  uint8_t show_heartbeat = 1;

  // check to see if knobs changed using getopt_long()
  int traces_encountered = 0;
  static struct option long_options[] = {{"warmup_instructions", required_argument, 0, 'w'},
                                         {"simulation_instructions", required_argument, 0, 'i'},
                                         {"hide_heartbeat", no_argument, 0, 'h'},
                                         {"cloudsuite", no_argument, 0, 'c'},
                                         {"traces", no_argument, &traces_encountered, 1},
                                         {0, 0, 0, 0}};

  int c;
  while ((c = getopt_long_only(argc, argv, "w:i:hc", long_options, NULL)) != -1 && !traces_encountered) {
    switch (c) {
    case 'w':
      warmup_instructions = atol(optarg);
      break;
    case 'i':
      simulation_instructions = atol(optarg);
      break;
    case 'h':
      show_heartbeat = 0;
      break;
    case 'c':
      knob_cloudsuite = 1;
      MAX_INSTR_DESTINATIONS = NUM_INSTR_DESTINATIONS_SPARC;
      break;
    case 0:
      break;
    default:
      abort();
    }
  }

  cout << "Warmup Instructions: " << warmup_instructions << endl;
  cout << "Simulation Instructions: " << simulation_instructions << endl;
  cout << "Number of CPUs: " << NUM_CPUS << endl;

  long long int dram_size = DRAM_CHANNELS * DRAM_RANKS * DRAM_BANKS * DRAM_ROWS * DRAM_COLUMNS * BLOCK_SIZE / 1024 / 1024; // in MiB
  std::cout << "Off-chip DRAM Size: ";
  if (dram_size > 1024)
    std::cout << dram_size / 1024 << " GiB";
  else
    std::cout << dram_size << " MiB";
  std::cout << " Channels: " << DRAM_CHANNELS << " Width: " << 8 * DRAM_CHANNEL_WIDTH << "-bit Data Rate: " << DRAM_IO_FREQ << " MT/s" << std::endl;

  std::cout << std::endl;
  std::cout << "VirtualMemory physical capacity: " << std::size(vmem.ppage_free_list) * vmem.page_size;
  std::cout << " num_ppages: " << std::size(vmem.ppage_free_list) << std::endl;
  std::cout << "VirtualMemory page size: " << PAGE_SIZE << " log2_page_size: " << LOG2_PAGE_SIZE << std::endl;

  std::cout << std::endl;
  for (int i = optind; i < argc; i++) {
    std::cout << "CPU " << traces.size() << " runs " << argv[i] << std::endl;

    traces.push_back(get_tracereader(argv[i], traces.size(), knob_cloudsuite));

    if (traces.size() > NUM_CPUS) {
      printf("\n*** Too many traces for the configured number of cores ***\n\n");
      assert(0);
    }
  }

  if (traces.size() != NUM_CPUS) {
    printf("\n*** Not enough traces for the configured number of cores ***\n\n");
    assert(0);
  }
  // end trace file setup

  // SHARED CACHE
  for (O3_CPU* cpu : ooo_cpu) {
    cpu->initialize_core();
  }

  for (auto it = caches.rbegin(); it != caches.rend(); ++it) {
    (*it)->impl_prefetcher_initialize();
    (*it)->impl_replacement_initialize();
  }

  // simulation entry point
  while (std::any_of(std::begin(simulation_complete), std::end(simulation_complete), std::logical_not<uint8_t>())) {

    uint64_t elapsed_second = (uint64_t)(time(NULL) - start_time), elapsed_minute = elapsed_second / 60, elapsed_hour = elapsed_minute / 60;
    elapsed_minute -= elapsed_hour * 60;
    elapsed_second -= (elapsed_hour * 3600 + elapsed_minute * 60);

    for (auto op : operables) {
      try {
        op->_operate();
      } catch (champsim::deadlock& dl) {
        // ooo_cpu[dl.which]->print_deadlock();
        // std::cout << std::endl;
        // for (auto c : caches)
        for (auto c : operables) {
          c->print_deadlock();
          std::cout << std::endl;
        }

        abort();
      }
    }
    std::sort(std::begin(operables), std::end(operables), champsim::by_next_operate());

    for (std::size_t i = 0; i < ooo_cpu.size(); ++i) {
      // read from trace
      while (ooo_cpu[i]->fetch_stall == 0 && ooo_cpu[i]->instrs_to_read_this_cycle > 0) {
        ooo_cpu[i]->init_instruction(traces[i]->get());
      }

      // heartbeat information
      if (show_heartbeat && (ooo_cpu[i]->num_retired >= ooo_cpu[i]->next_print_instruction)) {
        float cumulative_ipc;
        if (warmup_complete[i])
          cumulative_ipc = (1.0 * (ooo_cpu[i]->num_retired - ooo_cpu[i]->begin_sim_instr)) / (ooo_cpu[i]->current_cycle - ooo_cpu[i]->begin_sim_cycle);
        else
          cumulative_ipc = (1.0 * ooo_cpu[i]->num_retired) / ooo_cpu[i]->current_cycle;
        float heartbeat_ipc = (1.0 * ooo_cpu[i]->num_retired - ooo_cpu[i]->last_sim_instr) / (ooo_cpu[i]->current_cycle - ooo_cpu[i]->last_sim_cycle);

        cout << "Heartbeat CPU " << i << " instructions: " << ooo_cpu[i]->num_retired << " cycles: " << ooo_cpu[i]->current_cycle;
        cout << " heartbeat IPC: " << heartbeat_ipc << " cumulative IPC: " << cumulative_ipc;
        cout << " (Simulation time: " << elapsed_hour << " hr " << elapsed_minute << " min " << elapsed_second << " sec) " << endl;
        ooo_cpu[i]->next_print_instruction += STAT_PRINTING_PERIOD;

        ooo_cpu[i]->last_sim_instr = ooo_cpu[i]->num_retired;
        ooo_cpu[i]->last_sim_cycle = ooo_cpu[i]->current_cycle;
      }

      // check for warmup
      // warmup complete
      if ((warmup_complete[i] == 0) && (ooo_cpu[i]->num_retired > warmup_instructions)) {
        warmup_complete[i] = 1;
        all_warmup_complete++;
      }
      if (all_warmup_complete == NUM_CPUS) { // this part is called only once
                                             // when all cores are warmed up
        all_warmup_complete++;
        finish_warmup();
      }

      // simulation complete
      if ((all_warmup_complete > NUM_CPUS) && (simulation_complete[i] == 0)
          && (ooo_cpu[i]->num_retired >= (ooo_cpu[i]->begin_sim_instr + simulation_instructions))) {
        simulation_complete[i] = 1;
        ooo_cpu[i]->finish_sim_instr = ooo_cpu[i]->num_retired - ooo_cpu[i]->begin_sim_instr;
        ooo_cpu[i]->finish_sim_cycle = ooo_cpu[i]->current_cycle - ooo_cpu[i]->begin_sim_cycle;

        cout << "Finished CPU " << i << " instructions: " << ooo_cpu[i]->finish_sim_instr << " cycles: " << ooo_cpu[i]->finish_sim_cycle;
        cout << " cumulative IPC: " << ((float)ooo_cpu[i]->finish_sim_instr / ooo_cpu[i]->finish_sim_cycle);
        cout << " (Simulation time: " << elapsed_hour << " hr " << elapsed_minute << " min " << elapsed_second << " sec) " << endl;

        for (auto it = caches.rbegin(); it != caches.rend(); ++it)
          record_roi_stats(i, *it);
      }
    }
  }

  uint64_t elapsed_second = (uint64_t)(time(NULL) - start_time), elapsed_minute = elapsed_second / 60, elapsed_hour = elapsed_minute / 60;
  elapsed_minute -= elapsed_hour * 60;
  elapsed_second -= (elapsed_hour * 3600 + elapsed_minute * 60);

  cout << endl << "ChampSim completed all CPUs" << endl;
  if (NUM_CPUS > 1) {
    cout << endl << "Total Simulation Statistics (not including warmup)" << endl;
    for (uint32_t i = 0; i < NUM_CPUS; i++) {
      cout << endl
           << "CPU " << i
           << " cumulative IPC: " << (float)(ooo_cpu[i]->num_retired - ooo_cpu[i]->begin_sim_instr) / (ooo_cpu[i]->current_cycle - ooo_cpu[i]->begin_sim_cycle);
      cout << " instructions: " << ooo_cpu[i]->num_retired - ooo_cpu[i]->begin_sim_instr
           << " cycles: " << ooo_cpu[i]->current_cycle - ooo_cpu[i]->begin_sim_cycle << endl;
      for (auto it = caches.rbegin(); it != caches.rend(); ++it)
        print_sim_stats(i, *it);
    }
  }

  cout << endl << "Region of Interest Statistics" << endl;
  for (uint32_t i = 0; i < NUM_CPUS; i++) {
    cout << endl << "CPU " << i << " cumulative IPC: " << ((float)ooo_cpu[i]->finish_sim_instr / ooo_cpu[i]->finish_sim_cycle);
    cout << " instructions: " << ooo_cpu[i]->finish_sim_instr << " cycles: " << ooo_cpu[i]->finish_sim_cycle << endl;
    for (auto it = caches.rbegin(); it != caches.rend(); ++it)
      print_roi_stats(i, *it);
  }

  for (auto it = caches.rbegin(); it != caches.rend(); ++it)
    (*it)->impl_prefetcher_final_stats();

<<<<<<< HEAD
  // ROB
  ooo_cpu[i].ROB.cpu = i;

  // BRANCH PREDICTOR
  ooo_cpu[i].initialize_branch_predictor();

  // TLBs
  ooo_cpu[i].ITLB.cpu = i;
  ooo_cpu[i].ITLB.cache_type = IS_ITLB;
  ooo_cpu[i].ITLB.MAX_READ = 2;
  ooo_cpu[i].ITLB.fill_level = FILL_L1;
  ooo_cpu[i].ITLB.extra_interface = &ooo_cpu[i].L1I;
  ooo_cpu[i].ITLB.lower_level = &ooo_cpu[i].STLB;

  ooo_cpu[i].DTLB.cpu = i;
  ooo_cpu[i].DTLB.cache_type = IS_DTLB;
  // ooo_cpu[i].DTLB.MAX_READ = (2 > MAX_READ_PER_CYCLE) ? MAX_READ_PER_CYCLE : 2;
  ooo_cpu[i].DTLB.MAX_READ = 2;
  ooo_cpu[i].DTLB.fill_level = FILL_L1;
  ooo_cpu[i].DTLB.extra_interface = &ooo_cpu[i].L1D;
  ooo_cpu[i].DTLB.lower_level = &ooo_cpu[i].STLB;

  ooo_cpu[i].STLB.cpu = i;
  ooo_cpu[i].STLB.cache_type = IS_STLB;
  ooo_cpu[i].STLB.MAX_READ = 1;
  ooo_cpu[i].STLB.fill_level = FILL_L2;
  ooo_cpu[i].STLB.upper_level_icache[i] = &ooo_cpu[i].ITLB;
  ooo_cpu[i].STLB.upper_level_dcache[i] = &ooo_cpu[i].DTLB;

  // PRIVATE CACHE
  ooo_cpu[i].L1I.cpu = i;
  ooo_cpu[i].L1I.cache_type = IS_L1I;
  // ooo_cpu[i].L1I.MAX_READ = (FETCH_WIDTH > MAX_READ_PER_CYCLE) ? MAX_READ_PER_CYCLE : FETCH_WIDTH;
  ooo_cpu[i].L1I.MAX_READ = 2;
  ooo_cpu[i].L1I.fill_level = FILL_L1;
  ooo_cpu[i].L1I.lower_level = &ooo_cpu[i].L2C;
  ooo_cpu[i].l1i_prefetcher_initialize();
  ooo_cpu[i].L1I.l1i_prefetcher_cache_operate = cpu_l1i_prefetcher_cache_operate;
  ooo_cpu[i].L1I.l1i_prefetcher_cache_fill = cpu_l1i_prefetcher_cache_fill;

  ooo_cpu[i].L1D.cpu = i;
  ooo_cpu[i].L1D.cache_type = IS_L1D;
  ooo_cpu[i].L1D.MAX_READ = (2 > MAX_READ_PER_CYCLE) ? MAX_READ_PER_CYCLE : 2;
  ooo_cpu[i].L1D.fill_level = FILL_L1;
  ooo_cpu[i].L1D.lower_level = &ooo_cpu[i].L2C;
  ooo_cpu[i].L1D.l1d_prefetcher_initialize();

  ooo_cpu[i].L2C.cpu = i;
  ooo_cpu[i].L2C.cache_type = IS_L2C;
  ooo_cpu[i].L2C.fill_level = FILL_L2;
  ooo_cpu[i].L2C.upper_level_icache[i] = &ooo_cpu[i].L1I;
  ooo_cpu[i].L2C.upper_level_dcache[i] = &ooo_cpu[i].L1D;
  ooo_cpu[i].L2C.lower_level = &uncore.LLC;
  ooo_cpu[i].L2C.l2c_prefetcher_initialize();

  // SHARED CACHE
  uncore.LLC.cache_type = IS_LLC;
  uncore.LLC.fill_level = FILL_LLC;
  uncore.LLC.MAX_READ = NUM_CPUS;
  uncore.LLC.upper_level_icache[i] = &ooo_cpu[i].L2C;
  uncore.LLC.upper_level_dcache[i] = &ooo_cpu[i].L2C;
  uncore.LLC.lower_level = &uncore.DRAM;

  // OFF-CHIP DRAM
  uncore.DRAM.fill_level = FILL_DRAM;
  uncore.DRAM.upper_level_icache[i] = &uncore.LLC;
  uncore.DRAM.upper_level_dcache[i] = &uncore.LLC;
  for (uint32_t i = 0; i < DRAM_CHANNELS; i++) {
    uncore.DRAM.RQ[i].is_RQ = 1;
    uncore.DRAM.WQ[i].is_WQ = 1;
  }

  warmup_complete[i] = 0;
  // all_warmup_complete = NUM_CPUS;
  simulation_complete[i] = 0;
  current_core_cycle[i] = 0;
  stall_cycle[i] = 0;

  previous_ppage = 0;
  num_adjacent_page = 0;
  num_cl[i] = 0;
  allocated_pages = 0;
  num_page[i] = 0;
  minor_fault[i] = 0;
  major_fault[i] = 0;
}

uncore.LLC.llc_initialize_replacement();
uncore.LLC.llc_prefetcher_initialize();

// simulation entry point
start_time = time(NULL);
uint8_t run_simulation = 1;
while (run_simulation) {

  uint64_t elapsed_second = (uint64_t)(time(NULL) - start_time), elapsed_minute = elapsed_second / 60, elapsed_hour = elapsed_minute / 60;
  elapsed_minute -= elapsed_hour * 60;
  elapsed_second -= (elapsed_hour * 3600 + elapsed_minute * 60);
  if (current_core_cycle[0] % 5000000 <= 10 && current_core_cycle[0] >= 5000000) { // Checking optimal partition after every 5 million cycles
    uncore.LLC.par = ucp_par();
  }
  for (int i = 0; i < NUM_CPUS; i++) {
    // proceed one cycle
    current_core_cycle[i]++;

    // cout << "Trying to process instr_id: " << ooo_cpu[i].instr_unique_id << " fetch_stall: " << +ooo_cpu[i].fetch_stall;
    // cout << " stall_cycle: " << stall_cycle[i] << " current: " << current_core_cycle[i] << endl;

    // core might be stalled due to page fault or branch misprediction
    if (stall_cycle[i] <= current_core_cycle[i]) {

      // retire
      if ((ooo_cpu[i].ROB.entry[ooo_cpu[i].ROB.head].executed == COMPLETED) && (ooo_cpu[i].ROB.entry[ooo_cpu[i].ROB.head].event_cycle <= current_core_cycle[i]))
        ooo_cpu[i].retire_rob();

      // complete
      ooo_cpu[i].update_rob();

      // schedule
      uint32_t schedule_index = ooo_cpu[i].ROB.next_schedule;
      if ((ooo_cpu[i].ROB.entry[schedule_index].scheduled == 0) && (ooo_cpu[i].ROB.entry[schedule_index].event_cycle <= current_core_cycle[i]))
        ooo_cpu[i].schedule_instruction();
      // execute
      ooo_cpu[i].execute_instruction();

      ooo_cpu[i].update_rob();

      // memory operation
      ooo_cpu[i].schedule_memory_instruction();
      ooo_cpu[i].execute_memory_instruction();

      ooo_cpu[i].update_rob();

      // decode
      if (ooo_cpu[i].DECODE_BUFFER.occupancy > 0) {
        ooo_cpu[i].decode_and_dispatch();
      }

      // fetch
      ooo_cpu[i].fetch_instruction();

      // read from trace
      if ((ooo_cpu[i].IFETCH_BUFFER.occupancy < ooo_cpu[i].IFETCH_BUFFER.SIZE) && (ooo_cpu[i].fetch_stall == 0)) {
        ooo_cpu[i].read_from_trace();
      }
    }

    // heartbeat information
    if (show_heartbeat && (ooo_cpu[i].num_retired >= ooo_cpu[i].next_print_instruction)) {
      float cumulative_ipc;
      if (warmup_complete[i])
        cumulative_ipc = (1.0 * (ooo_cpu[i].num_retired - ooo_cpu[i].begin_sim_instr)) / (current_core_cycle[i] - ooo_cpu[i].begin_sim_cycle);
      else
        cumulative_ipc = (1.0 * ooo_cpu[i].num_retired) / current_core_cycle[i];
      float heartbeat_ipc = (1.0 * ooo_cpu[i].num_retired - ooo_cpu[i].last_sim_instr) / (current_core_cycle[i] - ooo_cpu[i].last_sim_cycle);

      cout << "Heartbeat CPU " << i << " instructions: " << ooo_cpu[i].num_retired << " cycles: " << current_core_cycle[i];
      cout << " heartbeat IPC: " << heartbeat_ipc << " cumulative IPC: " << cumulative_ipc;
      cout << " (Simulation time: " << elapsed_hour << " hr " << elapsed_minute << " min " << elapsed_second << " sec) " << endl;
      ooo_cpu[i].next_print_instruction += STAT_PRINTING_PERIOD;

      ooo_cpu[i].last_sim_instr = ooo_cpu[i].num_retired;
      ooo_cpu[i].last_sim_cycle = current_core_cycle[i];
    }

    // check for deadlock
    if (ooo_cpu[i].ROB.entry[ooo_cpu[i].ROB.head].ip && (ooo_cpu[i].ROB.entry[ooo_cpu[i].ROB.head].event_cycle + DEADLOCK_CYCLE) <= current_core_cycle[i])
      print_deadlock(i);

    // check for warmup
    // warmup complete
    if ((warmup_complete[i] == 0) && (ooo_cpu[i].num_retired > warmup_instructions)) {
      warmup_complete[i] = 1;
      all_warmup_complete++;
    }
    if (all_warmup_complete == NUM_CPUS) { // this part is called only once when all cores are warmed up
      all_warmup_complete++;
      finish_warmup();
    }

    /*
    if (all_warmup_complete == 0) {
        all_warmup_complete = 1;
        finish_warmup();
    }
    if (ooo_cpu[1].num_retired > 0)
        warmup_complete[1] = 1;
    */

    // simulation complete
    if ((all_warmup_complete > NUM_CPUS) && (simulation_complete[i] == 0)
        && (ooo_cpu[i].num_retired >= (ooo_cpu[i].begin_sim_instr + ooo_cpu[i].simulation_instructions))) {
      simulation_complete[i] = 1;
      ooo_cpu[i].finish_sim_instr = ooo_cpu[i].num_retired - ooo_cpu[i].begin_sim_instr;
      ooo_cpu[i].finish_sim_cycle = current_core_cycle[i] - ooo_cpu[i].begin_sim_cycle;

      cout << "Finished CPU " << i << " instructions: " << ooo_cpu[i].finish_sim_instr << " cycles: " << ooo_cpu[i].finish_sim_cycle;
      cout << " cumulative IPC: " << ((float)ooo_cpu[i].finish_sim_instr / ooo_cpu[i].finish_sim_cycle);
      cout << " (Simulation time: " << elapsed_hour << " hr " << elapsed_minute << " min " << elapsed_second << " sec) " << endl;

      record_roi_stats(i, &ooo_cpu[i].L1D);
      record_roi_stats(i, &ooo_cpu[i].L1I);
      record_roi_stats(i, &ooo_cpu[i].L2C);
      record_roi_stats(i, &uncore.LLC);

      all_simulation_complete++;
    }

    if (all_simulation_complete == NUM_CPUS)
      run_simulation = 0;
  }

  // TODO: should it be backward?
  uncore.DRAM.operate();
  uncore.LLC.operate();
}

uint64_t elapsed_second = (uint64_t)(time(NULL) - start_time), elapsed_minute = elapsed_second / 60, elapsed_hour = elapsed_minute / 60;
elapsed_minute -= elapsed_hour * 60;
elapsed_second -= (elapsed_hour * 3600 + elapsed_minute * 60);

cout << endl << "ChampSim completed all CPUs" << endl;
if (NUM_CPUS > 1) {
  cout << endl << "Total Simulation Statistics (not including warmup)" << endl;
  for (uint32_t i = 0; i < NUM_CPUS; i++) {
    cout << endl
         << "CPU " << i
         << " cumulative IPC: " << (float)(ooo_cpu[i].num_retired - ooo_cpu[i].begin_sim_instr) / (current_core_cycle[i] - ooo_cpu[i].begin_sim_cycle);
    cout << " instructions: " << ooo_cpu[i].num_retired - ooo_cpu[i].begin_sim_instr << " cycles: " << current_core_cycle[i] - ooo_cpu[i].begin_sim_cycle
         << endl;
#ifndef CRC2_COMPILE
    print_sim_stats(i, &ooo_cpu[i].L1D);
    print_sim_stats(i, &ooo_cpu[i].L1I);
    print_sim_stats(i, &ooo_cpu[i].L2C);
    ooo_cpu[i].l1i_prefetcher_final_stats();
    ooo_cpu[i].L1D.l1d_prefetcher_final_stats();
    ooo_cpu[i].L2C.l2c_prefetcher_final_stats();
#endif
    print_sim_stats(i, &uncore.LLC);
  }
  uncore.LLC.llc_prefetcher_final_stats();
}

cout << endl << "Region of Interest Statistics" << endl;
for (uint32_t i = 0; i < NUM_CPUS; i++) {
  cout << endl << "CPU " << i << " cumulative IPC: " << ((float)ooo_cpu[i].finish_sim_instr / ooo_cpu[i].finish_sim_cycle);
  cout << " instructions: " << ooo_cpu[i].finish_sim_instr << " cycles: " << ooo_cpu[i].finish_sim_cycle << endl;
#ifndef CRC2_COMPILE
  print_roi_stats(i, &ooo_cpu[i].L1D);
  print_roi_stats(i, &ooo_cpu[i].L1I);
  print_roi_stats(i, &ooo_cpu[i].L2C);
#endif
  print_roi_stats(i, &uncore.LLC);
  cout << "Major fault: " << major_fault[i] << " Minor fault: " << minor_fault[i] << endl;
}

for (uint32_t i = 0; i < NUM_CPUS; i++) {
  ooo_cpu[i].l1i_prefetcher_final_stats();
  ooo_cpu[i].L1D.l1d_prefetcher_final_stats();
  ooo_cpu[i].L2C.l2c_prefetcher_final_stats();
}

uncore.LLC.llc_prefetcher_final_stats();
=======
  for (auto it = caches.rbegin(); it != caches.rend(); ++it)
    (*it)->impl_replacement_final_stats();
>>>>>>> 29f568cfe2a6067b3a125d63742e37be0622d626

#ifndef CRC2_COMPILE print_dram_stats();
print_branch_stats();
#endif

return 0;
}
