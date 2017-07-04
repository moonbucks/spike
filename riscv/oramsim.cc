#include "oramsim.h"
#include "common.h"
#include <cstdlib>
#include <iostream>
#include <iomanip>
#include <cmath>
#define DEBUG 1

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

  for (uint64_t i=0; i< stashsz; i++) {
    stash[i] = 0;
  }

  // all the blocks are mapped into leaves
  uint64_t leafid, bid;
  for (leafid = tree_leaves, bid = 0; leafid <= tree_nodes; leafid++, bid++) {
    oram_tree[leafid] = bid;
    position_map[bid] = leafid;
  }

  // ---------------------------------------------------------------------

  oram_accesses = 0;
  stash_hit = 0;
  stash_miss = 0;
  write_accesses = 0;
  read_accesses = 0;

  if (DEBUG) {
    std::cout << "init() " << std::endl;

    std::cout << "printing position map : bid -> leaf_id" << std::endl;
    for (bid = 0; bid < 16384; bid++) {
        std::cout << "block " << std::dec << bid << " -> " << position_map[bid] << std::endl;
    }

  }
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

void oram_sim_t::print_path(uint64_t leaf)
{
    std::cout << "Printing path to leaf" << std::dec << leaf << std::endl ; 
    std::cout << "| " ;
}

void oram_sim_t::print_stash()
{
    int i;
    std::cout << "Printing stash,,," << std::endl ; 
    std::cout << "| " ;
    for (i=0; i<(int)stashsz; i++) {
        uint64_t d = stash[i];
        if (d == (uint64_t)-1) std::cout << std::dec << "-1" << " | ";
        else std::cout << std::dec << stash[i] << " | ";
    }
    std::cout << std::endl;
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
    if (DEBUG) std::cout << "stash write back.. " << std::endl;

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

    if (DEBUG) {
        std::cout << "nearest ancestor node id: " << std::dec << nearest_ancestor << std::endl;
        std::cout << std::dec << "nearest ancestor height: " << cur_height << std::endl;
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

    if (DEBUG) {
        std::cout << "After reshuffling the stash, ";
        print_stash();
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

    if (DEBUG) {
        std::cout << "After bring evicted blocks, ";
        print_stash();
    }

    if (!found) {
        // TODO Here is a bug
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

    if (stash_overflow) std::cout << "Stash overflow! " << std::endl;
    assert( !stash_overflow );

    // write back to oram tree and clear the stash on index 0..tree_height
    int cur, stack_cursor; 
    for (stack_cursor= tree_height, cur=leaf; stack_cursor>=0; stack_cursor--) {
        oram_tree[cur] = stash[stack_cursor];
        stash[stack_cursor] = -1;
        std::cout << std::dec << "sid: " << stack_cursor << ", cur_node: " << cur 
            << " contains block: " << (int64_t)oram_tree[cur] << std::endl;

        cur = cur / 2;
    } 

    if (DEBUG) {
        std::cout << "After bring evicted blocks, ";
        print_stash();
    }

}

void oram_sim_t::access(uint64_t addr, size_t bytes, bool store)
{
    oram_accesses++;
    uint64_t block_id = get_block_id(addr);

    assert ( block_id >= 0 );
    assert ( block_id < 16384 ); 

    uint64_t leaf, new_leaf;

    if ( block_is_in_stash(block_id) ) { 
        if (DEBUG) std::cout << "stash hit at " << std::hex << addr << "(bid=" <<std::dec << block_id << ")" << std::endl;
        stash_hit++;
    }
    else {
        if (DEBUG) std::cout << "stash miss at " << std::hex << addr << std::endl;
        stash_miss++;

        // find leaf id from position map
        leaf = position_map[block_id];
        if (DEBUG) std::cout << std::hex << addr << "(bid=" << std::dec << block_id << ") stored in leaf id: " << std::dec << leaf << " (leaf range: " << tree_leaves << " ~ " << tree_nodes << " ) " << std::endl; 

        // assign to a random new leaf
        new_leaf = rand() % tree_leaves + tree_leaves;
        position_map[block_id] = new_leaf;
        if (DEBUG) std::cout << std::hex << addr << "(bid=" << std::dec << block_id << ") assigned to a new leaf: " << std::dec <<  new_leaf << " (leaf range: " << tree_leaves << " ~ " << tree_nodes << ")" <<  std::endl;

        // load the entire path to stash
        load_path_to_stash(leaf);

        if(DEBUG) { 
            std::cout << "Path to leaf: " << std::dec << leaf << " is loaded to stash " << std::endl;
            print_stash();
        }

        // not "access++", but "request++"
        // store ? write_accesses++ : read_accesses++;
        
        stash_write_back(leaf, block_id, new_leaf);
    }
}
