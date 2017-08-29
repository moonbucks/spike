
interface mem_intf(input logic clk, reset);

	logic [7:0] random_address;
	logic [7:0] random_requested_address;
	logic [7:0] random_read_data;
	logic [7:0] random_write_data;
	logic we;
	logic re;
	logic oe;

endinterface