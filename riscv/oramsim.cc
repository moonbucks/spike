#include "oramsim.h"
#include "common.h"
#include <cstdlib>
#include <iostream>
#include <iomanip>
#include <cmath>

oram_sim_t::oram_sim_t(size_t _blocksz, size_t _stashsz)
 : blocksz(_blocksz), stashsz(_stashsz)
{
  init();
}

static void help()
{
  exit(1);
}

oram_sim_t* oram_sim_t::construct(const char* config)
{
  const char* wp = strchr(config, ':');
  if (!wp++) help();

  size_t blocksz = atoi(std::string(config, wp).c_str());
  size_t stashsz = atoi(wp);

  // hardcoded
  stashsz = 20; 
  tree_height = 14;
  tree_leaves = pow(2, tree_height); 
  tree_nodes = pow(2, tree_height+1) - 1; 

  return new oram_sim_t(blocksz, stashsz);
}

void oram_sim_t::init()
{
  stash = new uint64_t[stashsz*sizeof(uint64_t)];

  oram_accesses = 0;
  stash_hit = 0;
  stash_miss = 0;
  write_accesses = 0;
  read_accesses = 0;

}

/*
uint64_t* oram_sim_t::check_block(uint64_t addr)
{
}
*/

oram_sim_t::~oram_sim_t()
{
  print_stats();
}

void oram_sim_t::print_stats()
{
  std::cout << "ORAM controller" << " ";
  std::cout << "ORAM Accesses:         " << oram_accesses << std::endl;
  std::cout << "ORAM controller" << " ";
  std::cout << "Stash Hit:             " << stash_hit << std::endl;
}

bool oram_sim_t::block_is_in_stash(uint64_t block_id) 
{
    int i;
    for (i=0; i<(int)stashsz; i++) {
        if (stash[i] == block_id) return true;
    }

    return false;
}

uint64_t oram_sim_t::get_block_id(uint64_t addr) 
{
    // hardcoded
    return (addr - 0x80000000) / 256;
}

void oram_sim_t::load_path_to_stash(uint64_t leaf) 
{
    // assert stash 0..height is always cleared
}

void oram_sim_t::stash_write_back(uint64_t leaf)
{

}

void oram_sim_t::access(uint64_t addr, size_t bytes, bool store)
{
    oram_accesses++;
    uint64_t block_id = get_block_id(addr);
    //assert ( block_id >= 0 );
    //assert ( block_id < 16384 ); 
    // ?? why not work??

    uint64_t leaf, new_leaf;

    if ( block_is_in_stash(block_id) ) 
        stash_hit++;
    else {
        stash_miss++;

        // find leaf id from position map
        leaf = position_map[block_id];

        // assign to a random new leaf
        new_leaf = rand() % tree_leaves;
        position_map[block_id] = new_leaf;

        // load the entire path to stash
        load_path_to_stash(leaf);

        store ? write_accesses++ : read_accesses++;
        
        stash_write_back(leaf);
    }
}
