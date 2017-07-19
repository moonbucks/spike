#ifndef _RISCV_ORAM_SIM_H
#define _RISCV_ORAM_SIM_H

#include <cstring>
#include <string>
#include <map>
#include <cstdint>
#include <assert.h>
#include <dramsim.h>

class oram_sim_t
{
 public:
  oram_sim_t(size_t bucketsz, size_t stashsz, size_t mem_mb, size_t cacheline_sz);
  virtual ~oram_sim_t();

  void access(uint64_t addr, size_t bytes, bool store);
  void print_stats();
  void set_dram_handler(dram_sim_t* dh) { dram_handler = dh; }

  static oram_sim_t* construct(const char* config, size_t mem_mb);

 protected:
  void print_stash();
  void print_path(uint64_t leaf);
  bool block_is_in_stash(uint64_t block_id);
  uint64_t get_block_id(uint64_t addr);
  uint64_t block_id_to_addr(uint64_t block_id); 
  void load_path_to_stash(uint64_t leaf);
  void stash_write_back(uint64_t leaf, uint64_t bid, uint64_t new_leaf);
  bool is_leaf(uint64_t leaf);

  dram_sim_t* dram_handler;

  size_t bucketsz;
  size_t stashsz;
  size_t mem_mb;
  size_t cacheline_sz;

  size_t num_of_blocks;

  uint64_t* stash;
  // store block_id temporary 

  std::map<uint64_t, uint64_t> position_map;
  // position map : <block_id, leaf_id> 

  size_t tree_height;
  size_t tree_leaves;
  size_t tree_nodes;
  std::map<uint64_t, uint64_t> oram_tree;
  // oram tree : <node_id, block_id> 
  // oram tree has 16384 nodes for 4MB memory
  // from 1..16384, oram_tree[i] return block id of corresponding node

  uint64_t oram_accesses;
  uint64_t stash_hit;
  uint64_t stash_miss;
  uint64_t write_accesses;
  uint64_t read_accesses;


  void init();
};



#endif
