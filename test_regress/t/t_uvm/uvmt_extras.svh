`define uvmt_register_cb(T, CB) \
  static function void uvmt_drop_and_reregister_cb(); \
    uvmt_callbacks#(T,CB)::drop_globals(); \
    m_register_cb_``CB = uvm_callbacks#(T,CB)::m_register_pair(`"T`",`"CB`"); \
  endfunction

class uvmt_queue #(type T=int);
  static function void drop_globals();
    uvm_pkg::uvm_queue#(T)::m_global_queue = null;
  endfunction
endclass

class uvmt_callbacks #(type T=uvm_pkg::uvm_object, type CB=uvm_pkg::uvm_callback);
  static function void drop_globals();
    uvm_pkg::uvm_callbacks#(T, CB)::m_tw_cb_q.delete();
    uvm_pkg::uvm_callbacks#(T, CB)::m_t_inst = null;
    uvm_pkg::uvm_callbacks#(T, CB)::m_inst = null;
  endfunction
endclass
