#ifndef _RISCV_DRAM_SIM_H
#define _RISCV_DRAM_SIM_H

#include <cstring>
#include <string>
#include <map>
#include <cstdint>
#include <assert.h>
#include "memtracer.h"
#include <DRAMSim.h>

class dram_sim_t 
{

 public:
  dram_sim_t() {}
  virtual ~dram_sim_t() {}

  void access(uint64_t addr, size_t bytes, access_type type) 
  {
    if (type == LOAD || type == STORE) dram->addTransaction(type == STORE, addr);
  }
  void set_dram(DRAMSim::MultiChannelMemorySystem* mem) { dram = mem; }

 protected:
  DRAMSim::MultiChannelMemorySystem *dram;

};


#endif
