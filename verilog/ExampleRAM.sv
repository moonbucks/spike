module ExampleRAM(address, wdata, raddress, rdata, clk, we, oe, re);
	input [7:0] address; 	// from handler
	input [7:0] wdata;	// from handler
	output logic [7:0] raddress;	// to handler
	output logic [7:0] rdata;	// to handler 
	input clk; 	// clock
	input we;	// write enable
	input re;	// read enable
	output logic oe; 	// output enable

//reg [7:0] mem[0:255] ;
typedef reg [7:0] mem_addr;
reg [7:0] mem[mem_addr];

always @ (posedge clk)
begin : MEM_WRITE
	if (we && !re) begin
		//$display("DRAM: Write Serviced\n");
		mem[address] = wdata;
		// TO check: No need to oe <= 0 ? 
	end
end

// memory read 
always @ (posedge clk) 
begin : MEM_READ
	if (!we && re) begin
		//$display("DRAM: Read Serviced\n");
		rdata	<= mem[address];
		raddress <= address;
		oe <= 1; 
	end else if (!we && !re) begin
		//$display("DRAM: Reset\n");
		rdata <= 0;
		raddress <= 0;
		oe <= 0;
	end
 end

endmodule
