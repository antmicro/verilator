// SPDX-License-Identifier: Apache-2.0
//
//----------------------------------------------------------------------
// Copyright 2007-2014 Mentor Graphics Corporation
// Copyright 2014 Semifore
// Copyright 2010-2014 Synopsys, Inc.
// Copyright 2007-2018 Cadence Design Systems, Inc.
// Copyright 2011-2012 AMD
// Copyright 2013-2018 NVIDIA Corporation
// Copyright 2012-2017 Cisco Systems, Inc.
//   All Rights Reserved Worldwide
//
//   Licensed under the Apache License, Version 2.0 (the
//   "License"); you may not use this file except in
//   compliance with the License.  You may obtain a copy of
//   the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in
//   writing, software distributed under the License is
//   distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
//   CONDITIONS OF ANY KIND, either express or implied.  See
//   the License for the specific language governing
//   permissions and limitations under the License.
//----------------------------------------------------------------------

//-!!!- NOTICE -------------------------------------------------------------!!!-
// This is a non-production-ready modified version of UVM intended for coverage
// testing purpouses
//-!!!----------------------------------------------------------------------!!!-

// Status: Partial
// * State

`ifdef UVM_ENABLE_DEPRECATED_API
  typedef class uvm_test_done_objection;
`endif

typedef class uvm_sequencer_base;

typedef class uvm_domain;
typedef class uvm_task_phase;

typedef class uvm_phase_cb;

typedef uvm_callbacks#(uvm_phase, uvm_phase_cb) uvm_phase_cb_pool /* @uvm-ieee 1800.2-2017 auto D.4.1*/   ;

// UVM ~
// @uvm-ieee 1800.2-2017 auto 9.3.1.2
class uvm_phase extends uvm_object;

  //`uvm_object_utils(uvm_phase)

  //UVM `uvm_register_cb(uvm_phase, uvm_phase_cb)


  //--------------------
  // Group -- NODOCS -- Construction
  //--------------------


  // @uvm-ieee 1800.2-2017 auto 9.3.1.3.1
  extern function new(string name="uvm_phase",
                      uvm_phase_type phase_type=UVM_PHASE_SCHEDULE,
                      uvm_phase parent=null);


  // @uvm-ieee 1800.2-2017 auto 9.3.1.3.2
  extern function uvm_phase_type get_phase_type();

  // @uvm-ieee 1800.2-2017 auto 9.3.1.3.3
  extern virtual function void set_max_ready_to_end_iterations(int max);

  // @uvm-ieee 1800.2-2017 auto 9.3.1.3.4
  // @uvm-ieee 1800.2-2017 auto 9.3.1.3.6
  extern virtual function int get_max_ready_to_end_iterations();

  // @uvm-ieee 1800.2-2017 auto 9.3.1.3.5
  extern static function void set_default_max_ready_to_end_iterations(int max);

  extern static function int get_default_max_ready_to_end_iterations();

  //-------------
  // Group -- NODOCS -- State
  //-------------


  // @uvm-ieee 1800.2-2017 auto 9.3.1.4.1
  extern function uvm_phase_state get_state();



  // @uvm-ieee 1800.2-2017 auto 9.3.1.4.2
  extern function int get_run_count();



  // @uvm-ieee 1800.2-2017 auto 9.3.1.4.3
  extern function uvm_phase find_by_name(string name, bit stay_in_scope=1);



  // @uvm-ieee 1800.2-2017 auto 9.3.1.4.4
  extern function uvm_phase find(uvm_phase phase, bit stay_in_scope=1);



  // @uvm-ieee 1800.2-2017 auto 9.3.1.4.5
  extern function bit is(uvm_phase phase);



  // @uvm-ieee 1800.2-2017 auto 9.3.1.4.6
  extern function bit is_before(uvm_phase phase);



  // @uvm-ieee 1800.2-2017 auto 9.3.1.4.7
  extern function bit is_after(uvm_phase phase);

  //--------------------------
  // Internal - Implementation
  //--------------------------

  // Implementation - Construction
  //------------------------------
  protected uvm_phase_type m_phase_type;
  protected uvm_phase      m_parent;     // our 'schedule' node [or points 'up' one level]
  uvm_phase                m_imp;        // phase imp to call when we execute this node

  // Implementation - State
  //-----------------------
  local uvm_phase_state    m_state;
  local int                m_run_count; // num times this phase has executed
  local process            m_phase_proc;
  local static int         m_default_max_ready_to_end_iters = 20;    // 20 is the initial value defined by 1800.2-2017 9.3.1.3.5
`ifndef UVM_ENABLE_DEPRECATED_API
  local
`endif
  int                      max_ready_to_end_iters = get_default_max_ready_to_end_iterations();
  int                      m_num_procs_not_yet_returned;
  extern function uvm_phase m_find_predecessor(uvm_phase phase, bit stay_in_scope=1, uvm_phase orig_phase=null);
  extern function uvm_phase m_find_successor(uvm_phase phase, bit stay_in_scope=1, uvm_phase orig_phase=null);
  extern function uvm_phase m_find_predecessor_by_name(string name, bit stay_in_scope=1, uvm_phase orig_phase=null);
  extern function uvm_phase m_find_successor_by_name(string name, bit stay_in_scope=1, uvm_phase orig_phase=null);
  extern function void m_print_successors();

  // Implementation - Callbacks
  //---------------------------
  // Provide the required component traversal behavior. Called by execute()
//UVM  virtual function void traverse(uvm_component comp,
//UVM                                 uvm_phase phase,
//UVM                                 uvm_phase_state state);
//UVM  endfunction
//UVM  // Provide the required per-component execution flow. Called by traverse()
//UVM  virtual function void execute(uvm_component comp,
//UVM                                 uvm_phase phase);
//UVM  endfunction

  // Implementation - Schedule
  //--------------------------
  protected bit  m_predecessors[uvm_phase];
  protected bit  m_successors[uvm_phase];
  protected uvm_phase m_end_node;
  // Track the currently executing real task phases (used for debug)
  static protected bit m_executing_phases[uvm_phase];
//UVM  function uvm_phase get_begin_node(); if (m_imp != null) return this; return null; endfunction
//UVM  function uvm_phase get_end_node();   return m_end_node; endfunction

  // Implementation - Synchronization
  //---------------------------------
  local uvm_phase m_sync[$];  // schedule instance to which we are synced

//UVM `ifdef UVM_ENABLE_DEPRECATED_API
//UVM   // In order to avoid raciness during static initialization,
//UVM   // the creation of the "phase done" objection has been
//UVM   // delayed until the first call to get_objection(), and all
//UVM   // internal APIs have been updated to call get_objection() instead
//UVM   // of referring to phase_done directly.
//UVM   //
//UVM   // So as to avoid potential null handle dereferences in user code
//UVM   // which was accessing the phase_done variable directly, the variable
//UVM   // was renamed, and made local.  This takes a difficult to debug
//UVM   // run-time error, and converts it into an easy to catch compile-time
//UVM   // error.
//UVM   //
//UVM   // Code which is broken due to the protection of phase_done should be
//UVM   // refactored to use the get_objection() method.  Note that this also
//UVM   // opens the door to virtual get_objection() code, as described in
//UVM   // https://accellera.mantishub.io/view.php?id=6260
//UVM   uvm_objection phase_done;
//UVM `else // !`ifdef UVM_ENABLE_DEPRECATED_API
//UVM   local uvm_objection phase_done;
//UVM `endif

  local int unsigned m_ready_to_end_count;

//UVM  function int unsigned get_ready_to_end_count();
//UVM     return m_ready_to_end_count;
//UVM  endfunction
//UVM
//UVM  extern local function void get_predecessors_for_successors(output bit pred_of_succ[uvm_phase]);
//UVM  extern local task m_wait_for_pred();

  // Implementation - Jumping
  //-------------------------
//UVM  local bit                m_jump_bkwd;
//UVM  local bit                m_jump_fwd;
//UVM  local uvm_phase          m_jump_phase;
//UVM  local bit                m_premature_end;
//UVM  extern function void clear(uvm_phase_state state = UVM_PHASE_DORMANT);
//UVM  extern function void clear_successors(
//UVM                             uvm_phase_state state = UVM_PHASE_DORMANT,
//UVM                             uvm_phase end_state=null);

  // Implementation - Overall Control
  //---------------------------------
  local static mailbox #(uvm_phase) m_phase_hopper = new();

//UVM  extern static task m_run_phases();
//UVM  extern local task  execute_phase();
//UVM  extern local function void m_terminate_phase();
//UVM  extern local function void m_print_termination_state();
//UVM  extern local task wait_for_self_and_siblings_to_drop();
//UVM  extern function void kill();
//UVM  extern function void kill_successors();

  // TBD add more useful debug
  //---------------------------------
  protected static bit m_phase_trace;
  local static bit m_use_ovm_run_semantic;


//UVM  function string convert2string();
//UVM  //return $sformatf("PHASE %s = %p",get_name(),this);
//UVM  string s;
//UVM    s = $sformatf("phase: %s parent=%s  pred=%s  succ=%s",get_name(),
//UVM                     (m_parent==null) ? "null" : get_schedule_name(),
//UVM                     m_aa2string(m_predecessors),
//UVM                     m_aa2string(m_successors));
//UVM    return s;
//UVM  endfunction
//UVM
//UVM  local function string m_aa2string(bit aa[uvm_phase]); // TBD tidy
//UVM    string s;
//UVM    int i;
//UVM    s = "'{ ";
//UVM    foreach (aa[ph]) begin
//UVM      uvm_phase n = ph;
//UVM      s = {s, (n == null) ? "null" : n.get_name(),
//UVM        (i == aa.num()-1) ? "" : ", "};
//UVM      i++;
//UVM    end
//UVM    s = {s, " }"};
//UVM    return s;
//UVM  endfunction

  function bit is_domain();
    return (m_phase_type == UVM_PHASE_DOMAIN);
  endfunction

  virtual function void m_get_transitive_children(ref uvm_phase phases[$]);
    foreach (m_successors[succ])
    begin
        phases.push_back(succ);
        succ.m_get_transitive_children(phases);
    end
  endfunction


//UVM   // @uvm-ieee 1800.2-2017 auto 9.3.1.7.1
//UVM   function uvm_objection get_objection();
//UVM      uvm_phase imp;
//UVM      uvm_task_phase tp;
//UVM      imp = get_imp();
//UVM      // Only nodes with a non-null uvm_task_phase imp have objections
//UVM      if ((get_phase_type() != UVM_PHASE_NODE) || (imp == null) || !$cast(tp, imp)) begin
//UVM 	return null;
//UVM      end
//UVM      if (phase_done == null) begin
//UVM `ifdef UVM_ENABLE_DEPRECATED_API
//UVM 	   if (get_name() == "run") begin
//UVM               phase_done = uvm_test_done_objection::get();
//UVM 	   end
//UVM 	   else begin
//UVM  `ifdef VERILATOR
//UVM               phase_done = uvm_objection::type_id_create({get_name(), "_objection"});
//UVM  `else
//UVM               phase_done = uvm_objection::type_id::create({get_name(), "_objection"});
//UVM  `endif
//UVM 	   end
//UVM `else // !UVM_ENABLE_DEPRECATED_API
//UVM  `ifdef VERILATOR
//UVM         phase_done = uvm_objection::type_id_create({get_name(), "_objection"});
//UVM  `else
//UVM 	phase_done = uvm_objection::type_id::create({get_name(), "_objection"});
//UVM  `endif
//UVM `endif // UVM_ENABLE_DEPRECATED_API
//UVM      end
//UVM
//UVM      return phase_done;
//UVM   endfunction // get_objection

  // UVM !
  extern function uvm_domain get_domain();
  extern function uvm_phase get_schedule(bit hier = 0);
  extern function void jump(uvm_phase phase);
  extern function void set_jump_phase(uvm_phase phase) ;
  extern function void end_prematurely() ;

  local bit                m_jump_bkwd;
  local bit                m_jump_fwd;
  local uvm_phase          m_jump_phase;
  local bit                m_premature_end;

  extern function void add(uvm_phase phase,
                           uvm_phase with_phase=null,
                           uvm_phase after_phase=null,
                           uvm_phase before_phase=null,
                           uvm_phase start_with_phase=null,
                           uvm_phase end_with_phase=null
                        );
endclass

// UVM 1:1
class uvm_phase_state_change extends uvm_object;

  `uvm_object_utils(uvm_phase_state_change)

  // Implementation -- do not use directly
  /* local */ uvm_phase       m_phase;/*  */
  /* local */ uvm_phase_state m_prev_state;
  /* local */ uvm_phase       m_jump_to;

  function new(string name = "uvm_phase_state_change");
    super.new(name);
  endfunction



  // @uvm-ieee 1800.2-2017 auto 9.3.2.2.1
  virtual function uvm_phase_state get_state();
    return m_phase.get_state();
  endfunction


  // @uvm-ieee 1800.2-2017 auto 9.3.2.2.2
  virtual function uvm_phase_state get_prev_state();
    return m_prev_state;
  endfunction


  // @uvm-ieee 1800.2-2017 auto 9.3.2.2.3
  function uvm_phase jump_to();
    return m_jump_to;
  endfunction

endclass

// UVM 1:1
class uvm_phase_cb extends uvm_callback;


  // @uvm-ieee 1800.2-2017 auto 9.3.3.2.1
  function new(string name="unnamed-uvm_phase_cb");
     super.new(name);
  endfunction : new


  // @uvm-ieee 1800.2-2017 auto 9.3.3.2.2
  virtual function void phase_state_change(uvm_phase phase,
                                           uvm_phase_state_change change);
  endfunction
endclass

//------------------------------------------------------------------------------
// IMPLEMENTATION
//------------------------------------------------------------------------------

// UVM ~
function uvm_phase::new(string name="uvm_phase",
                        uvm_phase_type phase_type=UVM_PHASE_SCHEDULE,
                        uvm_phase parent=null);
  super.new(name);
  m_phase_type = phase_type;

  // The common domain is the only thing that initializes m_state.  All
  // other states are initialized by being 'added' to a schedule.
  if ((name == "common") &&
      (phase_type == UVM_PHASE_DOMAIN))
    m_state = UVM_PHASE_DORMANT;

  m_run_count = 0;
  m_parent = parent;

  //UVM begin
  //UVM   uvm_cmdline_processor clp = uvm_cmdline_processor::get_inst();
  //UVM   string val;
  //UVM   if (clp.get_arg_value("+UVM_PHASE_TRACE", val))
  //UVM     m_phase_trace = 1;
  //UVM   else
  //UVM     m_phase_trace = 0;
  //UVM   if (clp.get_arg_value("+UVM_USE_OVM_RUN_SEMANTIC", val))
  //UVM     m_use_ovm_run_semantic = 1;
  //UVM   else
  //UVM     m_use_ovm_run_semantic = 0;
  //UVM end
  m_phase_trace = 0;
  m_use_ovm_run_semantic = 0;
  //UVM



  if (parent == null && (phase_type == UVM_PHASE_SCHEDULE ||
                         phase_type == UVM_PHASE_DOMAIN )) begin
    //m_parent = this;
    m_end_node = new({name,"_end"}, UVM_PHASE_TERMINAL, this);
    this.m_successors[m_end_node] = 1;
    m_end_node.m_predecessors[this] = 1;
  end

endfunction

// UVM 1:1
function uvm_phase_type uvm_phase::get_phase_type();
  return m_phase_type;
endfunction

// UVM 1:1
function void uvm_phase::set_max_ready_to_end_iterations(int max);
  max_ready_to_end_iters = max;
endfunction

// UVM 1:1
function int uvm_phase::get_max_ready_to_end_iterations();
  return max_ready_to_end_iters;
endfunction

// UVM 1:1
function void uvm_phase::set_default_max_ready_to_end_iterations(int max);
  m_default_max_ready_to_end_iters = max;
endfunction

// UVM 1:1
function int uvm_phase::get_default_max_ready_to_end_iterations();
  return m_default_max_ready_to_end_iters;
endfunction

// UVM 1:1
function uvm_phase_state uvm_phase::get_state();
  return m_state;
endfunction

// UVM 1:1
function int uvm_phase::get_run_count();
  return m_run_count;
endfunction

// UVM 1:1
function void uvm_phase::m_print_successors();
  uvm_phase found;
  static string spaces = "                                                 ";
  static int level;
  if (m_phase_type == UVM_PHASE_DOMAIN)
    level = 0;
  `uvm_info("UVM/PHASE/SUCC",$sformatf("%s%s (%s) id=%0d",spaces.substr(0,level*2),get_name(), m_phase_type.name(),get_inst_id()),UVM_NONE)
  level++;
  foreach (m_successors[succ]) begin
    succ.m_print_successors();
  end
  level--;
endfunction

// UVM 1:1
function uvm_phase uvm_phase::m_find_predecessor(uvm_phase phase, bit stay_in_scope=1, uvm_phase orig_phase=null);
  uvm_phase found;
  //$display("  FIND PRED node '",phase.get_name(),"' (id=",$sformatf("%0d",phase.get_inst_id()),") - checking against ",get_name()," (",m_phase_type.name()," id=",$sformatf("%0d",get_inst_id()),(m_imp==null)?"":{"/",$sformatf("%0d",m_imp.get_inst_id())},")");
  if (phase == null) begin
    return null ;
  end
  if (phase == m_imp || phase == this)
    return this;
  foreach (m_predecessors[pred]) begin
    uvm_phase orig;
    orig = (orig_phase==null) ? this : orig_phase;
    if (!stay_in_scope ||
        (pred.get_schedule() == orig.get_schedule()) ||
        (pred.get_domain() == orig.get_domain())) begin
      found = pred.m_find_predecessor(phase,stay_in_scope,orig);
      if (found != null)
        return found;
    end
  end
  return null;
endfunction

// UVM 1:1
function uvm_phase uvm_phase::m_find_predecessor_by_name(string name, bit stay_in_scope=1, uvm_phase orig_phase=null);
  uvm_phase found;
  //$display("  FIND PRED node '",name,"' - checking against ",get_name()," (",m_phase_type.name()," id=",$sformatf("%0d",get_inst_id()),(m_imp==null)?"":{"/",$sformatf("%0d",m_imp.get_inst_id())},")");
  if (get_name() == name)
    return this;
  foreach (m_predecessors[pred]) begin
    uvm_phase orig;
    orig = (orig_phase==null) ? this : orig_phase;
    if (!stay_in_scope ||
        (pred.get_schedule() == orig.get_schedule()) ||
        (pred.get_domain() == orig.get_domain())) begin
      found = pred.m_find_predecessor_by_name(name,stay_in_scope,orig);
      if (found != null)
        return found;
    end
  end
  return null;
endfunction

// UVM 1:1
function uvm_phase uvm_phase::m_find_successor(uvm_phase phase, bit stay_in_scope=1, uvm_phase orig_phase=null);
  uvm_phase found;
  //$display("  FIND SUCC node '",phase.get_name(),"' (id=",$sformatf("%0d",phase.get_inst_id()),") - checking against ",get_name()," (",m_phase_type.name()," id=",$sformatf("%0d",get_inst_id()),(m_imp==null)?"":{"/",$sformatf("%0d",m_imp.get_inst_id())},")");
  if (phase == null) begin
    return null ;
  end
  if (phase == m_imp || phase == this) begin
    return this;
    end
  foreach (m_successors[succ]) begin
    uvm_phase orig;
    orig = (orig_phase==null) ? this : orig_phase;
    if (!stay_in_scope ||
        (succ.get_schedule() == orig.get_schedule()) ||
        (succ.get_domain() == orig.get_domain())) begin
      found = succ.m_find_successor(phase,stay_in_scope,orig);
      if (found != null) begin
        return found;
        end
    end
  end
  return null;
endfunction

// UVM 1:1
function uvm_phase uvm_phase::m_find_successor_by_name(string name, bit stay_in_scope=1, uvm_phase orig_phase=null);
  uvm_phase found;
  //$display("  FIND SUCC node '",name,"' - checking against ",get_name()," (",m_phase_type.name()," id=",$sformatf("%0d",get_inst_id()),(m_imp==null)?"":{"/",$sformatf("%0d",m_imp.get_inst_id())},")");
  if (get_name() == name)
    return this;
  foreach (m_successors[succ]) begin
    uvm_phase orig;
    orig = (orig_phase==null) ? this : orig_phase;
    if (!stay_in_scope ||
        (succ.get_schedule() == orig.get_schedule()) ||
        (succ.get_domain() == orig.get_domain())) begin
      found = succ.m_find_successor_by_name(name,stay_in_scope,orig);
      if (found != null)
        return found;
    end
  end
  return null;
endfunction

// UVM 1:1
function uvm_phase uvm_phase::find(uvm_phase phase, bit stay_in_scope=1);
  // TBD full search
  //$display({"\nFIND node '",phase.get_name(),"' within ",get_name()," (scope ",m_phase_type.name(),")", (stay_in_scope) ? " staying within scope" : ""});
  if (phase == m_imp || phase == this)
    return phase;
  find = m_find_predecessor(phase,stay_in_scope,this);
  if (find == null)
    find = m_find_successor(phase,stay_in_scope,this);
endfunction

// UVM 1:1
function uvm_phase uvm_phase::find_by_name(string name, bit stay_in_scope=1);
  // TBD full search
  //$display({"\nFIND node named '",name,"' within ",get_name()," (scope ",m_phase_type.name(),")", (stay_in_scope) ? " staying within scope" : ""});
  if (get_name() == name)
    return this;
  find_by_name = m_find_predecessor_by_name(name,stay_in_scope,this);
  if (find_by_name == null)
    find_by_name = m_find_successor_by_name(name,stay_in_scope,this);
endfunction

// UVM 1:1
function bit uvm_phase::is(uvm_phase phase);
  return (m_imp == phase || this == phase);
endfunction

// UVM 1:1
function bit uvm_phase::is_before(uvm_phase phase);
  //$display("this=%s is before phase=%s?",get_name(),phase.get_name());
  // TODO: add support for 'stay_in_scope=1' functionality
  return (!is(phase) && m_find_successor(phase,0,this) != null);
endfunction

// UVM 1:1
function bit uvm_phase::is_after(uvm_phase phase);
  //$display("this=%s is after phase=%s?",get_name(),phase.get_name());
  // TODO: add support for 'stay_in_scope=1' functionality
  return (!is(phase) && m_find_predecessor(phase,0,this) != null);
endfunction

// UVM 1:1
function uvm_domain uvm_phase::get_domain();
  uvm_phase phase;
  phase = this;
  while (phase != null && phase.m_phase_type != UVM_PHASE_DOMAIN)
    phase = phase.m_parent;
  if (phase == null) // no parent domain
    return null;
  if(!$cast(get_domain,phase))
      `uvm_fatal("PH/INTERNAL", "get_domain: m_phase_type is DOMAIN but $cast to uvm_domain fails")
endfunction

// UVM 1:1
function uvm_phase uvm_phase::get_schedule(bit hier=0);
  uvm_phase sched;
  sched = this;
  if (hier)
    while (sched.m_parent != null && (sched.m_parent.get_phase_type() == UVM_PHASE_SCHEDULE))
      sched = sched.m_parent;
  if (sched.m_phase_type == UVM_PHASE_SCHEDULE)
    return sched;
  if (sched.m_phase_type == UVM_PHASE_NODE)
    if (m_parent != null && m_parent.m_phase_type != UVM_PHASE_DOMAIN)
      return m_parent;
  return null;
endfunction

// UVM 1:1
function void uvm_phase::end_prematurely() ;
   m_premature_end = 1 ;
endfunction

// UVM 1:1
function void uvm_phase::jump(uvm_phase phase);
   set_jump_phase(phase) ;
   end_prematurely() ;
endfunction

// UVM 1:1
function void uvm_phase::set_jump_phase(uvm_phase phase) ;
  uvm_phase d;

  if ((m_state <  UVM_PHASE_STARTED) ||
      (m_state >  UVM_PHASE_ENDED) )
  begin
   `uvm_error("JMPPHIDL", { "Attempting to jump from phase \"",
      get_name(), "\" which is not currently active (current state is ",
      m_state.name(), "). The jump will not happen until the phase becomes ",
      "active."})
  end



  // A jump can be either forward or backwards in the phase graph.
  // If the specified phase (name) is found in the set of predecessors
  // then we are jumping backwards.  If, on the other hand, the phase is in the set
  // of successors then we are jumping forwards.  If neither, then we
  // have an error.
  //
  // If the phase is non-existant and thus we don't know where to jump
  // we have a situation where the only thing to do is to uvm_report_fatal
  // and terminate_phase.  By calling this function the intent was to
  // jump to some other phase. So, continuing in the current phase doesn't
  // make any sense.  And we don't have a valid phase to jump to.  So we're done.

  d = m_find_predecessor(phase,0);
  if (d == null) begin
    d = m_find_successor(phase,0);
    if (d == null) begin
      string msg;
      $sformat(msg,{"phase %s is neither a predecessor or successor of ",
                    "phase %s or is non-existant, so we cannot jump to it.  ",
                    "Phase control flow is now undefined so the simulation ",
                    "must terminate"}, phase.get_name(), get_name());
      `uvm_fatal("PH_BADJUMP", msg)
    end
    else begin
      m_jump_fwd = 1;
      `uvm_info("PH_JUMPF",$sformatf("jumping forward to phase %s", phase.get_name()),
                UVM_DEBUG)
    end
  end
  else begin
    m_jump_bkwd = 1;
    `uvm_info("PH_JUMPB",$sformatf("jumping backward to phase %s", phase.get_name()),
              UVM_DEBUG)
  end

  m_jump_phase = d;
endfunction

// UVM 1:1
function void uvm_phase::add(uvm_phase phase,
                             uvm_phase with_phase=null,
                             uvm_phase after_phase=null,
                             uvm_phase before_phase=null,
                             uvm_phase start_with_phase=null,
                             uvm_phase end_with_phase=null
                          );
  uvm_phase new_node, begin_node, end_node, tmp_node;
  uvm_phase_state_change state_chg;

  if (phase == null)
      `uvm_fatal("PH/NULL", "add: phase argument is null")

  if (with_phase != null && with_phase.get_phase_type() == UVM_PHASE_IMP) begin
    string nm = with_phase.get_name();
    with_phase = find(with_phase);
    if (with_phase == null)
      `uvm_fatal("PH_BAD_ADD",
         {"cannot find with_phase '",nm,"' within node '",get_name(),"'"})
  end

  if (before_phase != null && before_phase.get_phase_type() == UVM_PHASE_IMP) begin
    string nm = before_phase.get_name();
    before_phase = find(before_phase);
    if (before_phase == null)
      `uvm_fatal("PH_BAD_ADD",
         {"cannot find before_phase '",nm,"' within node '",get_name(),"'"})
  end

  if (after_phase != null && after_phase.get_phase_type() == UVM_PHASE_IMP) begin
    string nm = after_phase.get_name();
    after_phase = find(after_phase);
    if (after_phase == null)
      `uvm_fatal("PH_BAD_ADD",
         {"cannot find after_phase '",nm,"' within node '",get_name(),"'"})
  end

  if (start_with_phase != null && start_with_phase.get_phase_type() == UVM_PHASE_IMP) begin
    string nm = start_with_phase.get_name();
    start_with_phase = find(start_with_phase);
    if (start_with_phase == null)
      `uvm_fatal("PH_BAD_ADD",
         {"cannot find start_with_phase '",nm,"' within node '",get_name(),"'"})
  end

  if (end_with_phase != null && end_with_phase.get_phase_type() == UVM_PHASE_IMP) begin
     string nm = end_with_phase.get_name();
     end_with_phase = find(end_with_phase);
     if (end_with_phase == null)
      `uvm_fatal("PH_BAD_ADD",
         {"cannot find end_with_phase '",nm,"' within node '",get_name(),"'"})
  end

  if (((with_phase != null) + (after_phase != null) + (start_with_phase != null)) > 1)
    `uvm_fatal("PH_BAD_ADD",
       "only one of with_phase/after_phase/start_with_phase may be specified as they all specify predecessor")

  if (((with_phase != null) + (before_phase != null) + (end_with_phase != null)) > 1)
    `uvm_fatal("PH_BAD_ADD",
       "only one of with_phase/before_phase/end_with_phase may be specified as they all specify successor")

  if (before_phase == this ||
     after_phase == m_end_node ||
     with_phase == m_end_node ||
     start_with_phase == m_end_node ||
     end_with_phase == m_end_node)
    `uvm_fatal("PH_BAD_ADD",
       "cannot add before begin node, after end node, or with end nodes")

  if (before_phase != null && after_phase != null) begin
    if (!after_phase.is_before(before_phase)) begin
      `uvm_fatal("PH_BAD_ADD",{"Phase '",before_phase.get_name(),
                 "' is not before phase '",after_phase.get_name(),"'"})
    end
  end

  if (before_phase != null && start_with_phase != null) begin
    if (!start_with_phase.is_before(before_phase)) begin
      `uvm_fatal("PH_BAD_ADD",{"Phase '",before_phase.get_name(),
                 "' is not before phase '",start_with_phase.get_name(),"'"})
    end
  end

  if (end_with_phase != null && after_phase != null) begin
    if (!after_phase.is_before(end_with_phase)) begin
       `uvm_fatal("PH_BAD_ADD",{"Phase '",end_with_phase.get_name(),
                 "' is not before phase '",after_phase.get_name(),"'"})
    end
  end

  // If we are inserting a new "leaf node"
  if (phase.get_phase_type() == UVM_PHASE_IMP) begin
//UVM    uvm_task_phase tp;
    new_node = new(phase.get_name(),UVM_PHASE_NODE,this);
    new_node.m_imp = phase;
    begin_node = new_node;
    end_node = new_node;

  end
  // We are inserting an existing schedule
  else begin
    begin_node = phase;
    end_node   = phase.m_end_node;
    phase.m_parent = this;
  end

  // If 'with_phase' is us, then insert node in parallel
  /*
  if (with_phase == this) begin
    after_phase = this;
    before_phase = m_end_node;
  end
  */

  // If no before/after/with specified, insert at end of this schedule
  if (with_phase==null && after_phase==null && before_phase==null &&
     start_with_phase==null && end_with_phase==null) begin
    before_phase = m_end_node;
  end


  if (m_phase_trace) begin
    uvm_phase_type typ = phase.get_phase_type();
    `uvm_info("PH/TRC/ADD_PH",
      {get_name()," (",m_phase_type.name(),") ADD_PHASE: phase=",phase.get_full_name()," (",
      typ.name(),", inst_id=",$sformatf("%0d",phase.get_inst_id()),")",
      " with_phase=",   (with_phase == null)   ? "null" : with_phase.get_name(),
      " start_with_phase=",   (start_with_phase == null)   ? "null" : start_with_phase.get_name(),
      " end_with_phase=",   (end_with_phase == null)   ? "null" : end_with_phase.get_name(),
      " after_phase=",  (after_phase == null)  ? "null" : after_phase.get_name(),
      " before_phase=", (before_phase == null) ? "null" : before_phase.get_name(),
      " new_node=",     (new_node == null)     ? "null" : {new_node.get_name(),
                                                           " inst_id=",
                                                           $sformatf("%0d",new_node.get_inst_id())},
      " begin_node=",   (begin_node == null)   ? "null" : begin_node.get_name(),
      " end_node=",     (end_node == null)     ? "null" : end_node.get_name()},UVM_DEBUG)
  end


  //
  // INSERT IN PARALLEL WITH 'WITH' PHASE
  if (with_phase != null) begin
    // all pre-existing predecessors to with_phase are predecessors to the new phase
    begin_node.m_predecessors = with_phase.m_predecessors;
    foreach (with_phase.m_predecessors[pred]) pred.m_successors[begin_node] = 1;
    // all pre-existing successors to with_phase are successors to this phase
    end_node.m_successors = with_phase.m_successors;
    foreach (with_phase.m_successors[succ]) succ.m_predecessors[end_node] = 1;
  end

  if (start_with_phase != null) begin
    // all pre-existing predecessors to start_with_phase are predecessors to the new phase
    begin_node.m_predecessors = start_with_phase.m_predecessors;
    foreach (start_with_phase.m_predecessors[pred]) begin
      pred.m_successors[begin_node] = 1;
    end
    // if not otherwise specified, successors for the new phase are the successors to the end of this schedule
    if (before_phase == null && end_with_phase == null) begin
      end_node.m_successors = m_end_node.m_successors ;
      foreach (m_end_node.m_successors[succ]) begin
        succ.m_predecessors[end_node] = 1;
      end
    end
  end

  if (end_with_phase != null) begin
     // all pre-existing successors to end_with_phase are successors to the new phase
    end_node.m_successors = end_with_phase.m_successors;
    foreach (end_with_phase.m_successors[succ]) begin
      succ.m_predecessors[end_node] = 1;
    end
    // if not otherwise specified, predecessors for the new phase are the predecessors to the start of this schedule
    if (after_phase == null && start_with_phase == null) begin
      begin_node.m_predecessors = this.m_predecessors ;
      foreach (this.m_predecessors[pred]) begin
        pred.m_successors[begin_node] = 1;
      end
    end
  end

  // INSERT BEFORE PHASE
  if (before_phase != null) begin
    // unless predecessors to this phase are otherwise specified,
    // pre-existing predecessors to before_phase move to be predecessors to the new phase
    if (after_phase == null && start_with_phase == null) begin
      foreach (before_phase.m_predecessors[pred]) begin
        pred.m_successors.delete(before_phase);
        pred.m_successors[begin_node] = 1;
      end
      begin_node.m_predecessors = before_phase.m_predecessors;
      before_phase.m_predecessors.delete();
    end
    // there is a special case if before and after used to be adjacent;
    // the new phase goes in-between them
    else if (before_phase.m_predecessors.exists(after_phase)) begin
      before_phase.m_predecessors.delete(after_phase);
    end

    // before_phase is now the sole successor of this phase
    before_phase.m_predecessors[end_node] = 1;
    end_node.m_successors.delete() ;
    end_node.m_successors[before_phase] = 1;

  end


  // INSERT AFTER PHASE
  if (after_phase != null) begin
    // unless successors to this phase are otherwise specified,
    // pre-existing successors to after_phase are now successors to this phase
    if (before_phase == null && end_with_phase == null) begin
      foreach (after_phase.m_successors[succ]) begin
       succ.m_predecessors.delete(after_phase);
       succ.m_predecessors[end_node] = 1;
      end
      end_node.m_successors = after_phase.m_successors;
      after_phase.m_successors.delete();
    end
    // there is a special case if before and after used to be adjacent;
    // the new phase goes in-between them
    else if (after_phase.m_successors.exists(before_phase)) begin
      after_phase.m_successors.delete(before_phase);
    end

    // after_phase is the sole predecessor of this phase
    after_phase.m_successors[begin_node] = 1;
    begin_node.m_predecessors.delete();
    begin_node.m_predecessors[after_phase] = 1;
  end



  // Transition nodes to DORMANT state
  if (new_node == null)
    tmp_node = phase;
  else
    tmp_node = new_node;

//UVM `ifdef VERILATOR
//UVM    state_chg = uvm_phase_state_change::type_id_create(tmp_node.get_name());
//UVM `else
   state_chg = uvm_phase_state_change::type_id::create(tmp_node.get_name());
//UVM `endif
  state_chg.m_phase = tmp_node;
  state_chg.m_jump_to = null;
  state_chg.m_prev_state = tmp_node.m_state;
  tmp_node.m_state = UVM_PHASE_DORMANT;
  `uvm_do_callbacks(uvm_phase, uvm_phase_cb, phase_state_change(tmp_node, state_chg))
endfunction
