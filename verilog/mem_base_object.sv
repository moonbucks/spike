`ifndef MEM_BASE_OBJECT_SV
`define MEM_BASE_OBJECT_SV

typedef enum {READ, WRITE} accessType;

class mem_base_object;
	bit [7:0] addr;
	bit [7:0] data;
	accessType rw; 	// READ=0, WRITE=1
endclass

`endif
