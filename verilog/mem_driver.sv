`include "mem_base_object.sv"

class mem_driver;
	int transactions;
	virtual intf vif;

function new(virtual intf vif);
	this.vif = vif;
	this.transactions = 0;
endfunction

task reset;
	wait(vif.reset);
	$display("Driver: Reset started\n");
	vif.original_address 	<= 0;
	vif.original_data 	<= 0;
	vif.requested_address 	<= 0;
	vif.requested_data 	<= 0;
	wait(!vif.reset);
	$display("Driver: Reset finished\n");
endtask

task drive_mem (mem_base_object object);
	begin
		@ (posedge vif.clk);
		vif.original_address	= object.addr;
		vif.original_data	= (object.rw == WRITE) ? object.data : 0;
		vif.re			= (object.rw == READ) ? 1 : 0;
		vif.we			= (object.rw == WRITE) ? 1 : 0;
		vif.rw			= object.rw;

		if (object.rw == WRITE) begin
			$display("Driver: Memory write-> Addr: %x, Data: %x\n", 
				object.addr, object.data);
		end else begin
			$display("Driver: Memory read-> Addr: %x\n", object.addr);
		end

		@ (posedge vif.clk); 
		vif.original_address	<= 0;
		vif.rw			<= READ;
		vif.original_data	<= 0;
		vif.re	<= 0;
		vif.we	<= 0;
	
		transactions++;
	end
endtask

endclass

