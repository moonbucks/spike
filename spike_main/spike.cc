// See LICENSE for license details.

#include "sim.h"
#include "mmu.h"
#include "gdbserver.h"
#include "cachesim.h"
#include "oramsim.h"
#include "dramsim.h"
#include "extension.h"
#include <dlfcn.h>
#include <fesvr/option_parser.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <string>
#include <memory>
#include <DRAMSim.h>

#define DRAM_DEBUG 1

static void help()
{
  fprintf(stderr, "usage: spike [host options] <target program> [target options]\n");
  fprintf(stderr, "Host Options:\n");
  fprintf(stderr, "  -p<n>                 Simulate <n> processors [default 1]\n");
  fprintf(stderr, "  -m<n>                 Provide <n> MiB of target memory [default 4096]\n");
  fprintf(stderr, "  -d                    Interactive debug mode\n");
  fprintf(stderr, "  -g                    Track histogram of PCs\n");
  fprintf(stderr, "  -l                    Generate a log of execution\n");
  fprintf(stderr, "  -h                    Print this help message\n");
  fprintf(stderr, "  -H                 Start halted, allowing a debugger to connect\n");
  fprintf(stderr, "  --isa=<name>          RISC-V ISA string [default %s]\n", DEFAULT_ISA);
  fprintf(stderr, "  --ic=<S>:<W>:<B>      Instantiate a cache model with S sets,\n");
  fprintf(stderr, "  --dc=<S>:<W>:<B>        W ways, and B-byte blocks (with S and\n");
  fprintf(stderr, "  --l2=<S>:<W>:<B>        B both powers of 2).\n");
  fprintf(stderr, "  --extension=<name>    Specify RoCC Extension\n");
  fprintf(stderr, "  --extlib=<name>       Shared library to load\n");
  fprintf(stderr, "  --gdb-port=<port>  Listen on <port> for gdb to connect\n");
  fprintf(stderr, "  --dump-config-string  Print platform configuration string and exit\n");
  exit(1);
}

int main(int argc, char** argv)
{
  bool debug = false;
  bool halted = false;
  bool histogram = false;
  bool log = false;
  bool dump_config_string = false;
  bool dramsim = false;
  size_t nprocs = 1;
  size_t mem_mb = 0;
  //size_t blocksz = 4; // default oram block size is 4;
  std::unique_ptr<icache_sim_t> ic;
  std::unique_ptr<dcache_sim_t> dc;
  std::unique_ptr<cache_sim_t> l2;
  std::unique_ptr<oram_sim_t> oram;
  std::unique_ptr<dram_sim_t> dram_handler;
  DRAMSim::MultiChannelMemorySystem *dram;
  std::function<extension_t*()> extension;
  const char* isa = DEFAULT_ISA;
  uint16_t gdb_port = 0;


  option_parser_t parser;
  parser.help(&help);
  parser.option('h', 0, 0, [&](const char* s){help();});
  parser.option('d', 0, 0, [&](const char* s){debug = true;});
  parser.option('g', 0, 0, [&](const char* s){histogram = true;});
  parser.option('l', 0, 0, [&](const char* s){log = true;});
  parser.option('p', 0, 1, [&](const char* s){nprocs = atoi(s);});
  parser.option('m', 0, 1, [&](const char* s){mem_mb = atoi(s);});
  //parser.option('o', 0, 1, [&](const char* s){blocksz = atoi(s); oram.reset(new oram_sim_t(blocksz, 20));}); 
  // I wanted to use --halted, but for some reason that doesn't work.
  parser.option('H', 0, 0, [&](const char* s){halted = true;});
  parser.option(0, "gdb-port", 1, [&](const char* s){gdb_port = atoi(s);});
  parser.option(0, "ic", 1, [&](const char* s){ic.reset(new icache_sim_t(s));});
  parser.option(0, "dc", 1, [&](const char* s){dc.reset(new dcache_sim_t(s));});
  parser.option(0, "l2", 1, [&](const char* s){l2.reset(cache_sim_t::construct(s, "L2$"));});
  parser.option(0, "o", 1, [&](const char* s){oram.reset(oram_sim_t::construct(s, mem_mb));}); 
  parser.option(0, "isa", 1, [&](const char* s){isa = s;});
  parser.option(0, "extension", 1, [&](const char* s){extension = find_extension(s);});
  parser.option(0, "dump-config-string", 0, [&](const char *s){dump_config_string = true;});
  parser.option(0, "dramsim", 0, [&](const char *s){dramsim = true;
    dram_handler.reset(new dram_sim_t());
    dram = DRAMSim::getMemorySystemInstance("/home/user/riscv/rocket-chip/riscv-tools/riscv-isa-sim/DRAMSim2/ini/DDR2_micron_16M_8b_x8_sg3E.ini", 
        "/home/user/riscv/rocket-chip/riscv-tools/riscv-isa-sim/DRAMSim2/system.ini", ".", 
        "/home/user/riscv/rocket-chip/riscv-tools/riscv-isa-sim/DRAMSim2/example_app", 16384); 

    /* // TODO Define and register callbacks if needed

    //dram = DRAMSim::getMemorySystemInstance("ini/DDR2_micron_16M_8b_x8_sg3E.ini", "system.ini", "..", "example_app", 16384); 
    TransactionCompleteCB *read_cb = new Callback<some_object, void, unsigned, uint64_t, uint64_t>(&obj, &some_object::read_complete); 
    TransactionCompleteCB *write_cb = new Callback<some_object, void, unsigned, uint64_t, uint64_t>(&obj, &some_object::write_complete);

    dram->RegisterCallbacks(read_cb, write_cb, power_callback);
    */
    });
  parser.option(0, "extlib", 1, [&](const char *s){
    void *lib = dlopen(s, RTLD_NOW | RTLD_GLOBAL);
    if (lib == NULL) {
      fprintf(stderr, "Unable to load extlib '%s': %s\n", s, dlerror());
      exit(-1);
    }
  });

  auto argv1 = parser.parse(argv);
  std::vector<std::string> htif_args(argv1, (const char*const*)argv + argc);
  sim_t s(isa, nprocs, mem_mb, halted, htif_args);
  std::unique_ptr<gdbserver_t> gdbserver;
  if (gdb_port) {
    gdbserver = std::unique_ptr<gdbserver_t>(new gdbserver_t(gdb_port, &s));
    s.set_gdbserver(&(*gdbserver));
  }

  if (dump_config_string) {
    printf("%s", s.get_config_string());
    return 0;
  }

  if (dramsim && DRAM_DEBUG) {
    printf("dram test\n");

    dram->addTransaction(false, 0x100001UL);
    dram->addTransaction(false, 1LL<<33 | 0x100001UL);
    for (int cycle=0; cycle<5; cycle++) dram->update();

    dram->addTransaction(true, 0x900012);
    for (int cycle=0; cycle<45; cycle++) dram->update(); 

    dram->printStats(true);
  }

  if (!*argv1)
    help();

  if (ic && l2) ic->set_miss_handler(&*l2);
  if (dc && l2) dc->set_miss_handler(&*l2);
  if (l2 && oram) l2->set_oram_handler(&*oram);
  if (dram_handler) oram->set_dram_handler(&*dram_handler);
  if (dram) dram_handler->set_dram(&*dram);

  for (size_t i = 0; i < nprocs; i++)
  {
    if (ic) s.get_core(i)->get_mmu()->register_memtracer(&*ic);
    if (dc) s.get_core(i)->get_mmu()->register_memtracer(&*dc);
    if (extension) s.get_core(i)->register_extension(extension());
  }

  s.set_debug(debug);
  s.set_log(log);
  s.set_histogram(histogram);
  s.run();


  for( int cycle=0; cycle<100000; cycle++) dram->update();
  
  dram->printStats(true);

  return 0;
  //return s.run();
}
