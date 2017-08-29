`include "interface.sv"
`include "mem_interface.sv"
`include "test.sv"

module testbench;

	bit clk;
	bit reset;
	bit oram_enabled;

	always #10 clk = ~clk;

	initial begin
		reset = 1;
		#10 reset = 0;
	end

	always #10 oram_enabled = 0;

	intf ex_intf(clk, reset, oram_enabled);
	mem_intf ex_memintf(clk, reset);
	//test ex_test(ex_intf);		// ORAM Test
	sequential_test ex_test(ex_intf);	// Sequential Test (Without ORAM)

	Handler DUT (
		.clk(ex_intf.clk),
		.reset(ex_intf.reset),
		.enabled(ex_intf.oram_enabled),
		.rw(ex_intf.rw),
		.o_we(ex_intf.we),
		.o_re(ex_intf.re),
		.o_oe(ex_intf.oe),
		.original_address(ex_intf.original_address),
		.original_data(ex_intf.original_data),
		.requested_address(ex_intf.requested_address),
		.requested_data(ex_intf.requested_data),
		.random_address(ex_memintf.random_address),
		.random_requested_address(ex_memintf.random_requested_address),
		.random_write_data(ex_memintf.random_write_data),
		.random_read_data(ex_memintf.random_read_data),
		.we(ex_memintf.we),
		.re(ex_memintf.re),
		.oe(ex_memintf.oe)
	);

	ExampleRAM RAM (
		.clk(ex_intf.clk),
		.address(ex_memintf.random_address),
		.raddress(ex_memintf.random_requested_address),
		.wdata(ex_memintf.random_write_data),
		.rdata(ex_memintf.random_read_data),
		.we(ex_memintf.we),
		.re(ex_memintf.re),
		.oe(ex_memintf.oe)
	);

endmodule