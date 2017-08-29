`include "mem_base_object.sv"

module Handler (
	input 	clk, 	
	input 	reset,
	input 	enabled,
	input 	rw,
	input 	o_we, 
	input	o_re,
	output logic o_oe,
	input 	[7:0] original_address,
	input 	[7:0] original_data,
	output 	logic [7:0] requested_address,
	output 	logic [7:0] requested_data,
	output logic 	[7:0] random_address,
	input logic	[7:0] random_requested_address,
	input logic	[7:0] random_read_data,
	output logic	[7:0] random_write_data,
	output logic we,
	output logic re,
	input logic oe);

	// ORAM data structure definition
	int stashsz=10;
	//mem_base_object stash [0:9];		// Fixed size stash
	int stash_idx=0;

	typedef reg [7:0] mem_addr;
	reg [7:0] position_map [0:511];
	reg [7:0] oram_tree [0:511];
/*
	int mem_mb = 4;
	int number_of_blocks = mem_mb * 1024 * 1024 / cacheline_sz / bucket_sz ; 
	int tree_height = $clog2(number_of_blocks);

	int tree_leaves = 2 ** tree_height;
	int tree_nodes = 2 ** (tree_height + 1) - 1;
*/
	int tree_height = 8;
	int tree_leaves = 256;
	int tree_nodes = 511;

	typedef enum {CONVERT, CONVERT_CLEARED, READ_CLEARED, STASH_DUMP, STASH_CLEARED, WB_CLEARED} pipeline;
	pipeline state;

	int lid, bid;
	initial begin
		state = WB_CLEARED;
		if (enabled) begin
			for (lid=tree_leaves, bid=0; lid <= tree_nodes; lid++, bid++) begin
				oram_tree[lid] = bid;
				position_map[bid] = lid;
			end
		end
	end

	mem_base_object request_backup_queue[$] = {};
	mem_base_object wb_queue[$] = {};
	mem_base_object stash[$] = {};
	int load_path_queue[$] = {};

	mem_base_object request_queue[$] = {};
	mem_base_object reply_queue[$] = {};
	mem_base_object request, request_to_mem;
	mem_base_object reply, reply_from_mem;
	mem_base_object oram_request, wb_request;

	// Internal signals
	int leaf_id;
	bit load_path;

	mem_base_object oram_current_request;

	always @(posedge o_re or posedge o_we)
	begin 
		if (enabled) begin
			request = new();
			request.addr = original_address;
			request.rw = (rw == READ) ? READ : WRITE;
			request_backup_queue.push_back(request); 

			leaf_id = position_map [original_address];
			load_path_queue.push_back(leaf_id);

 			load_path = 1; 
			//state = CONVERT;
		end else begin
		// ORAM is not enabled
		if (rw == READ) begin
			request = new();
			request.addr = original_address;
			request.rw 	= READ;
			request_queue.push_back(request);
			//$display("Handler: DRAM Read Request Enqueued\n");
		end else begin
			request = new();
			request.addr = original_address;
			request.data	= original_data;
			request.rw	= WRITE;
			request_queue.push_back(request);
			//$display("Handler: DRAM Write Request Enqueued\n");
		end

		end
	end

	int s, cur, leaf;

	always @(posedge load_path) // this doesn't fit in the syntax
	begin
			if (load_path_queue.size() > 0 && state == WB_CLEARED) begin
				// need to generate log N requests 
				stash_idx =0;
				leaf = load_path_queue.pop_front();
				oram_current_request = request_backup_queue.pop_front();

				state = CONVERT;
				for (s = tree_height, cur = leaf; s >= 0; s--) begin
					oram_request = new();
					oram_request.addr = oram_tree[cur];
					oram_request.rw = READ; // always READ
					request_queue.push_back(oram_request);
				end
				state = CONVERT_CLEARED;
			end
	end


	always @(posedge clk)
	begin
	    if (request_queue.size() > 0) begin
		request_to_mem = request_queue.pop_front();
		random_address <= request_to_mem.addr;
		if (request_to_mem.rw == READ) begin
			random_write_data <= 0;
			re <= 1;
			we <= 0;
			//$display("Handler: DRAM Read Request Poped (Sent)\n");
		end else begin
			random_write_data <= request_to_mem.data;
			re <= 0;
			we <= 1;
			//$display("Handler: DRAM Write Request Poped (Sent)\n");
		end
	    end else if (state == CONVERT) begin
		// Wait until all requests are enqueued, but clear the signal
		re <= 0;
		we <= 0;
	    end else if (state == CONVERT_CLEARED) begin
		state = READ_CLEARED;
		re <= 0;
		we <= 0;
	    end else if (state == STASH_CLEARED) begin
		state = WB_CLEARED;
		re <= 0;
		we <= 0;
	    end else begin
		re <= 0;
		we <= 0;
	    end
	end

	always @(posedge oe)
	begin
		if (enabled) begin
			reply_from_mem = new();
			reply_from_mem.addr = random_requested_address;
			reply_from_mem.data = random_read_data;
			//stash[stash_idx] = reply_from_mem;
			stash.push_back(reply_from_mem);
			stash_idx += 1;
		end else begin
			reply_from_mem = new();
			reply_from_mem.addr = random_requested_address;
			reply_from_mem.data = random_read_data;
			reply_queue.push_back(reply_from_mem);
			//$display("Handler: DRAM Read Reply Received\n");
		end
	end

	always @(posedge clk) 
	begin
		if (enabled && stash.size() > 0) begin
			wb_request = stash.pop_front();
			if (wb_request.addr == oram_current_request.addr) begin
				reply_queue.push_back(wb_request);
			end 
			wb_request.rw = WRITE;
			wb_queue.push_back(wb_request);
		end
	end

	always @(posedge clk)
	begin

		if (state == READ_CLEARED && wb_queue.size() > 0) begin
			state = STASH_DUMP;
			while ( wb_queue.size() > 0) begin
				request = wb_queue.pop_front();
				request_queue.push_back(request);
			end
			state = STASH_CLEARED;
		end	
	end

	always @(posedge clk)
	begin
	    if (reply_queue.size() > 0) begin
		reply = reply_queue.pop_front();
		requested_address <= reply.addr;
		requested_data <= reply.data;
		o_oe <= 1;
		$display("Handler: Request Return: Addr: %x, Requested data: %x\n", requested_address, requested_data);
	    end else begin
		o_oe <= 0;
	    end
	end

	
endmodule
