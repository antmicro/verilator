typedef enum
{
  UVM_NONE   = 0,
  UVM_LOW    = 100,
  UVM_MEDIUM = 200,
  UVM_HIGH   = 300,
  UVM_FULL   = 400,
  UVM_DEBUG  = 500
} uvm_verbosity;

typedef enum bit [1:0]
{
  UVM_INFO,
  UVM_WARNING,
  UVM_ERROR,
  UVM_FATAL
} uvm_severity;

`define uvm_info(ID_, MSG, VERBOSITY) \
  $display("(mock) INFO(%d): [ID: %s]: %s ", VERBOSITY, ID_, MSG);

`define uvm_warning(ID_, MSG) \
  $display("(mock) WARNING [ID: %s]: %s", ID_, MSG);

`define uvm_error(ID_, MSG) \
  $display("(mock) ERROR [ID: %s]: %s", ID_, MSG);

`define uvm_fatal(ID_, MSG) \
  begin \
    $display("(mock) FATAL ERROR [ID: %s]: %s", ID_, MSG); \
    $stop(); \
  end

function static int uvm_report_enabled(int a, int b, string c);
  return 1'b1;
endfunction
