`include "environment.sv"

module test(intf ex_intf);
	environment env;

	initial begin
		env = new(ex_intf);

		env.generator.repeat_count = 5;
		env.run();
	end
endmodule

module sequential_test(intf ex_intf);
	environment env;

	initial begin
		env = new(ex_intf);

		env.generator.repeat_count = 5;
		env.run_sequential();
	end
endmodule