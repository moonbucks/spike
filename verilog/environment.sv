`include "mem_txgen.sv"
`include "op_monitor.sv"
//`include "mem_driver.sv"

class environment;
	mem_txgen generator;
	mem_driver driver;
	mem_monitor monitor;

	virtual intf vif;
	int repeat_count = 5;

	// constructor
	function new(virtual intf vif);
		this.vif = vif;

		driver = new(vif);
		generator = new(driver);
		monitor = new(vif, repeat_count);
	endfunction

	task test_reset();
		driver.reset();
	endtask

	task test();
		fork
		generator.main();
		//driver.main();
		//join_any
		join
	endtask

	task test_sequential();
		fork
		generator.main_sequential();
		monitor.monitoring();
		join_any
	endtask

	task test_terminate();
		wait(generator.ended.triggered);
		$display("Environment: generator.ended.triggered\n");
		wait(generator.repeat_count *2 == driver.transactions);
		$display("Environment: generator.generated == driver.transactions\n");
		wait(generator.repeat_count == monitor.transactions);
		$display("Environment: generator.generated == monitor.monitored. All READs are served\n");
	endtask

	task run;
		test_reset();
		test();
		test_terminate();
		$display("finished\n");
		$finish;
	endtask

	task run_sequential;
		test_reset();
		test_sequential();
		test_terminate();
		$display("finished sequential\n");
		$finish;
	endtask
endclass