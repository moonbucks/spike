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
  return new oram_sim_t(blocksz, stashsz);
}

void oram_sim_t::init()
{
  stash = new uint64_t[stashsz*sizeof(uint64_t)];

  tree_height = 14;
  tree_leaves = pow(2, tree_height); // 16384
  tree_nodes = pow(2, tree_height+1) - 1; // 32767 

  // TODO add randomness on initial assignment --------------------------

  // node id starts from 1..nodes. at first, nonleaf node is empty
  for (uint64_t i = 1; i < tree_leaves; i++) {
    oram_tree[i] = -1;
  }

  // all the blocks are mapped into leaves
  uint64_t leafid, bid;
  for (leafid = tree_leaves, bid = 0; leafid <= tree_nodes; leafid++, bid++) {
    oram_tree[leafid] = bid;
    position_map[bid] = leafid;
    bid++;
  }

  // ---------------------------------------------------------------------

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

bool oram_sim_t::is_leaf(uint64_t leaf)
{
    return leaf >= tree_leaves && leaf <= tree_nodes;
}

void oram_sim_t::load_path_to_stash(uint64_t leaf) 
{
    int i, s, cur;

    assert ( is_leaf(leaf) );
    //assert ( stash[0..tree_height] is cleared );

    for (s= tree_height, cur=leaf; s>=0; s--) {
        stash[s] = oram_tree[cur];
        cur = cur / 2;
    }

}

void oram_sim_t::stash_write_back(uint64_t leaf, uint64_t bid, uint64_t new_leaf)
{
    int cur_height, nearest_ancestor=1;
    int nid_from_new_leaf, nid_from_cur_leaf;

    // find nearest ancestor
    for (cur_height=tree_height, nid_from_new_leaf = new_leaf, nid_from_cur_leaf = leaf;
        cur_height >= 0; cur_height--) {
        if (nid_from_new_leaf == nid_from_cur_leaf ) {
            nearest_ancestor = nid_from_cur_leaf;
            break;
        }

        nid_from_new_leaf /= 2;
        nid_from_cur_leaf /= 2; 
        
    }

    uint64_t sid, ssid;

    // remove current block from stash
    for ( sid=0; sid <stashsz; sid++) {
        if (stash[sid] == bid) {
            stash[sid] = -1;
            break;
        }
    }
 
    bool found = false, stash_overflow = false; 
    // check stash if there is an empty bucket and move the block to new place
    for ( sid= cur_height; sid>=0; sid--) {
        if (stash[sid] == (uint64_t) -1) {
            stash[sid] = bid;
            found = true;
            break;
        }
    }


    // bring blocks from evicted blocks
    for (sid= tree_height; sid < stashsz; sid++) {
        if (position_map [ stash[sid] ] == leaf) {
            for (ssid= 0; ssid < tree_height; ssid++) {
                if (stash[ssid] == (uint64_t) -1) {
                    stash[ssid] = stash[sid];
                    stash[sid] == (uint64_t) -1;
                    break;
                }
            }
        }
    }

    if (!found) {
        // this block cannot be write back. leave this block to stash
        for (sid= tree_height; sid < stashsz; sid++) {
            if (stash[sid] == (uint64_t) -1) {
                stash[sid] = bid;
                break;
            } 

            if (sid == stashsz-1) {
                stash_overflow = true;
            }
        }
    }

    if (stash_overflow) {
        // TODO abort..  
    }

    // write back to oram tree and clear the stash on index 0..tree_height
    int cur; 
    for (sid= tree_height, cur=leaf; sid>=0; sid--) {
        oram_tree[cur] = stash[sid];
        stash[sid] = -1;
        cur = cur / 2;
    } 

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
        new_leaf = rand() % tree_leaves + tree_leaves;
        position_map[block_id] = new_leaf;

        // load the entire path to stash
        load_path_to_stash(leaf);

        // not "access++", but "request++"
        // store ? write_accesses++ : read_accesses++;
        
        stash_write_back(leaf, block_id, new_leaf);
    }
}
