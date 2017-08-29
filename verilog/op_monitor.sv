`include "mem_base_object.sv"

class mem_monitor;
	mem_base_object mem_object;
	virtual intf vif;

	int transactions;
	int monitor_counts;
	event ended;

function new(virtual intf vif, int count);
	this.vif = vif;
	this.monitor_counts = count;
	this.transactions = 0;
endfunction

task monitoring();
	begin
		while(transactions < monitor_counts) begin @ (posedge vif.oe);
			
			//mem_object = new();
			$display("Monitor: Memory LD -> Address: %x, Data: %x (transaction=%x)\n", 
				vif.requested_address, vif.requested_data, transactions +1 );
			//mem_object.addr = vif.requested_address;
			//mem_object.data = vif.requested_data;
			transactions++;
		end
	end
	->ended;
endtask

/*task monitoring();
	begin
		while(1) begin @ (negedge vif.clock);
			if (vif.rw == READ) begin
				mem_object = new();
				$display("Monitor: Memory LD -> Address: %x, Data: %x\n", 
					vif.requested_address, vif.requested_data);
				mem_object.addr = vif.requested_address;
				mem_object.data = vif.requested_data;

			end
		end
	end
endtask*/

endclass
