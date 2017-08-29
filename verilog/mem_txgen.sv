`include "mem_base_object.sv"
`include "mem_driver.sv"

class mem_txgen;
	mem_base_object mem_object;
	mem_driver driver;

	int repeat_count;
	event ended;

function new(mem_driver driver);
	this.driver = driver;
endfunction

task main_sequential();
	begin
		int i=0;
		int prev_addr = 0;

		// write first
		for (i=0; i<repeat_count; i++) begin
			$display("TXGEN: %d th sequential transaction generated.\n", i);
			mem_object = new();
			mem_object.addr = prev_addr;
			mem_object.data = prev_addr;
			mem_object.rw = WRITE;
			driver.drive_mem(mem_object);
			prev_addr += 16;
		end
		// read
		prev_addr = 0;
		for (i=0; i<repeat_count; i++) begin
			$display("TXGEN: %d th sequential transaction generated\n", i);
			mem_object = new();
			mem_object.addr = prev_addr;
			mem_object.data = 0;
			mem_object.rw = READ;
			driver.drive_mem(mem_object);
			prev_addr += 16;
		end
	end
	->ended;
endtask

task main();
   	begin
		int i=0;
		for (i=0; i<repeat_count; i++) begin
			$display("TXGEN: %d th random transaction generated\n", i);
			mem_object = new();
			mem_object.addr = $random();
			mem_object.data = $random();
			// basic read / write
			mem_object.rw = WRITE;
			driver.drive_mem(mem_object);
			mem_object.rw = READ;
			driver.drive_mem(mem_object);
		end	
		-> ended; // triggering
    	end
endtask

endclass


/*
`ifndef MEM_TXGEN_SV
`define MEM_TXGEN_SV

`include "mem_base_object.sv"
`include "mem_driver.sv"

class mem_txgen;
	mem_base_object mem_object;
	mem_driver mem_driver;

	int num_requests;

function new(virtual mem_port port);
	begin
		num_requests = 10;
		mem_driver = new(port);
	end
endfunction

task gen_request();
	begin
		int i=0;
		for (i=0; i<num_requests; i++) begin
			mem_object = new();
			mem_object.addr = $random();
			mem_object.data = $random();
			// basic read / write
			mem_object.rw = WRITE;
			mem_driver.drive_mem(mem_object);
			mem_object.rw = READ;
			mem_driver.drive_mem(mem_object);
		end
	end
endtask

endclass
`endif
*/