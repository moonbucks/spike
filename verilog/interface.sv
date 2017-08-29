interface intf(input logic clk, reset, oram_enabled);

	logic [7:0] original_address;
	logic [7:0] original_data;
	logic [7:0] requested_address;
	logic [7:0] requested_data;
	logic we;
	logic re;
	logic oe;
	logic rw;

endinterface
