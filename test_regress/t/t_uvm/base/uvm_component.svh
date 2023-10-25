// SPDX-License-Identifier: Apache-2.0
//
//------------------------------------------------------------------------------
// Copyright 2010 Paradigm Works
// Copyright 2007-2017 Mentor Graphics Corporation
// Copyright 2014 Semifore
// Copyright 2018 Intel Corporation
// Copyright 2010-2014 Synopsys, Inc.
// Copyright 2007-2018 Cadence Design Systems, Inc.
// Copyright 2011-2018 AMD
// Copyright 2012-2018 Cisco Systems, Inc.
// Copyright 2013-2018 NVIDIA Corporation
// Copyright 2012 Accellera Systems Initiative
// Copyright 2017-2018 Verific
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
//------------------------------------------------------------------------------

//-!!!- NOTICE -------------------------------------------------------------!!!-
// This is a non-production-ready modified version of UVM intended for coverage
// testing purpouses
//-!!!----------------------------------------------------------------------!!!-

// Status: Partial implementation
// * Hierarchical interface

/* typedef class uvm_objection;
typedef class uvm_sequence_base;
typedef class uvm_sequence_item; */

// @uvm-ieee 1800.2-2017 auto 13.1.1
//UVM virtual class uvm_component extends uvm_report_object;
virtual class uvm_component extends uvm_object;
//UVM
  extern function new (string name, uvm_component parent); // UVM: Test OK (t_uvm_component_hierarchical)
  extern virtual function uvm_component get_parent (); // UVM: Test OK (t_uvm_component_hierarchical)
  extern virtual function string get_full_name (); // UVM: Test OK (t_uvm_component_hierarchical)
  extern local virtual function void m_set_full_name();
  extern function void get_children(ref uvm_component children[$]); // UVM: Test OK (t_uvm_component_hierarchical)
  extern function uvm_component get_child (string name); // UVM: Test OK (t_uvm_component_hierarchical)
  extern function int get_next_child (ref string name); // UVM: Test OK (t_uvm_component_hierarchical)
  extern function int get_first_child (ref string name); // UVM: Test OK (t_uvm_component_hierarchical)
  extern function int get_num_children (); // UVM: Test OK (t_uvm_component_hierarchical)
  extern function int has_child (string name); // UVM: Test OK (t_uvm_component_hierarchical)
  extern virtual function void set_name (string name);
  extern function uvm_component lookup (string name); // UVM: Test OK (t_uvm_component_hierarchical)
  extern function int unsigned get_depth(); // UVM: Test OK (t_uvm_component_hierarchical)


  //----------------------------------------------------------------------------
  //                     PRIVATE or PSUEDO-PRIVATE members
  //                      *** Do not call directly ***
  //         Implementation and even existence are subject to change.
  //----------------------------------------------------------------------------
  // Most local methods are prefixed with m_, indicating they are not
  // user-level methods. SystemVerilog does not support friend classes,
  // which forces some otherwise internal methods to be exposed (i.e. not
  // be protected via 'local' keyword). These methods are also prefixed
  // with m_ to indicate they are not intended for public use.
  //
  // Internal methods will not be documented, although their implementa-
  // tions are freely available via the open-source license.
  //----------------------------------------------------------------------------

`ifdef UVM_ENABLE_DEPRECATED_API
  extern                   function void set_int_local(string field_name,
                                                       uvm_bitstream_t value,
                                                       bit recurse=1);
`endif // UVM_ENABLE_DEPRECATED_API
  //UVM extern                   function void set_local(uvm_resource_base rsrc) ;

  /*protected*/ uvm_component m_parent;
  protected     uvm_component m_children[string];
  protected     uvm_component m_children_by_handle[uvm_component];
  extern protected virtual function bit  m_add_child(uvm_component child);
  //UVM extern local     virtual function void ();

  //UVM extern                   function void do_resolve_bindings();
  //UVM extern                   function void do_flush();

  //UVM extern virtual           function void flush ();

  extern local             function void m_extract_name(string name ,
                                                        output string leaf ,
                                                        output string remainder );

  // overridden to disable
  //UVM extern virtual function uvm_object create (string name="");
  //UVM extern virtual function uvm_object clone  ();

  //UVM local uvm_tr_stream m_streams[string][string];
  //UVM local uvm_recorder m_tr_h[uvm_transaction];
  //UVM extern protected function int m_begin_tr (uvm_transaction tr,
  //UVM                                               int parent_handle=0,
  //UVM                                               string stream_name="main", string label="",
  //UVM                                               string desc="", time begin_time=0);

  string m_name;

 //UVM  typedef uvm_abstract_component_registry#(uvm_component, "uvm_component") type_id;
 //UVM  `uvm_type_name_decl("uvm_component")

  //UVM protected uvm_event_pool event_pool;

  //UVM int unsigned recording_detail = UVM_NONE;
  //UVM extern         function void   do_print(uvm_printer printer);

  // Internal methods for setting up command line messaging stuff
  //UVM extern function void m_set_cl_msg_args;
  //UVM extern function void m_set_cl_verb;
  //UVM extern function void m_set_cl_action;
  //UVM extern function void m_set_cl_sev;
  //UVM extern function void m_apply_verbosity_settings(uvm_phase phase);

  // The verbosity settings may have a specific phase to start at.
  // We will do this work in the phase_started callback.

  typedef struct {
    string comp;
    string phase;
    time   offset;
    uvm_verbosity verbosity;
    string id;
  } m_verbosity_setting;

  m_verbosity_setting m_verbosity_settings[$];
  static m_verbosity_setting m_time_settings[$];

  // does the pre abort callback hierarchically
  //UVM extern /*local*/ function void m_do_pre_abort;

  // produce message for unsupported types from apply_config_settings
  //UVM uvm_resource_base m_unsupported_resource_base = null;
  //UVM extern function void m_unsupported_set_local(uvm_resource_base rsrc);

  typedef struct  {
    string arg;
    string args[$];
    int unsigned used;
  } uvm_cmdline_parsed_arg_t;

  static uvm_cmdline_parsed_arg_t m_uvm_applied_cl_action[$];
  static uvm_cmdline_parsed_arg_t m_uvm_applied_cl_sev[$];

endclass

//------------------------------------------------------------------------------
// IMPLEMENTATION
//------------------------------------------------------------------------------

// UVM ~
function uvm_component::new (string name, uvm_component parent);
  string error_str;
  uvm_root top;
  uvm_coreservice_t cs;

  super.new(name);

  // If uvm_top, reset name to "" so it doesn't show in full paths then return
  if (parent==null && name == "__top__") begin
    set_name(""); // *** VIRTUAL
    return;
  end

  cs = uvm_coreservice_t::get();
  top = cs.get_root();

  // Check that we're not in or past end_of_elaboration
  //UVM begin
  //UVM   uvm_phase bld;
  //UVM   uvm_domain common;
  //UVM   common = uvm_domain::get_common_domain();
  //UVM   bld = common.find(uvm_build_phase::get());
  //UVM   if (bld == null)
  //UVM     uvm_report_fatal("COMP/INTERNAL",
  //UVM                      "attempt to find build phase object failed",UVM_NONE);
  //UVM   if (bld.get_state() == UVM_PHASE_DONE) begin
  //UVM     uvm_report_fatal("ILLCRT", {"It is illegal to create a component ('",
  //UVM               name,"' under '",
  //UVM               (parent == null ? top.get_full_name() : parent.get_full_name()),
  //UVM              "') after the build phase has ended."},
  //UVM                      UVM_NONE);
  //UVM   end
  //UVM end

  if (name == "") begin
    name.itoa(m_inst_count);
    name = {"COMP_", name};
  end

  if(parent == this) begin
    `uvm_fatal("THISPARENT", "cannot set the parent of a component to itself")
  end

  if (parent == null)
    parent = top;

  if(uvm_report_enabled(UVM_MEDIUM+1, UVM_INFO, "NEWCOMP"))
    `uvm_info("NEWCOMP", {"Creating ",
      (parent==top?"uvm_top":parent.get_full_name()),".",name},UVM_MEDIUM+1)

  if (parent.has_child(name) && this != parent.get_child(name)) begin
    if (parent == top) begin
      error_str = {"Name '",name,"' is not unique to other top-level ",
      "instances. If parent is a module, build a unique name by combining the ",
      "the module name and component name: $sformatf(\"\%m.\%s\",\"",name,"\")."};
      `uvm_fatal("CLDEXT",error_str)
    end
    else
      `uvm_fatal("CLDEXT",
        $sformatf("Cannot set '%s' as a child of '%s', %s",
                  name, parent.get_full_name(),
                  "which already has a child by that name."))
    return;
  end

  m_parent = parent;

  set_name(name); // *** VIRTUAL

  if (!m_parent.m_add_child(this))
    m_parent = null;

  //UVM event_pool = new("event_pool");

  //UVM m_domain = parent.m_domain;     // by default, inherit domains from parents

  // Now that inst name is established, reseed (if use_uvm_seeding is set)
  //UVM reseed();

  // Do local configuration settings
  //UVM if (!uvm_config_db #(uvm_bitstream_t)::get(this, "", "recording_detail", recording_detail))
  //UVM       void'(uvm_config_db #(int)::get(this, "", "recording_detail", recording_detail));

  //UVM m_rh.set_name(get_full_name());
  //UVM set_report_verbosity_level(parent.get_report_verbosity_level());

  //UVM m_set_cl_msg_args();
endfunction

function uvm_component uvm_component::get_parent();
  return  m_parent;
endfunction

// UVM 1:1
function string uvm_component::get_full_name();
  // Note- Implementation choice to construct full name once since the
  // full name may be used often for lookups.
  if(m_name == "")
    return get_name();
  else
    return m_name;
endfunction

// UVM 1:1
function void uvm_component::get_children(ref uvm_component children[$]);
  foreach(m_children[i])
    children.push_back(m_children[i]);
endfunction

// UVM 1:1
function uvm_component uvm_component::get_child(string name);
  if (m_children.exists(name))
    return m_children[name];
  `uvm_warning("NOCHILD",{"Component with name '",name,
       "' is not a child of component '",get_full_name(),"'"})
  return null;
endfunction

// UVM 1:1
function int uvm_component::get_next_child(ref string name);
  return m_children.next(name);
endfunction

// UVM 1:1
function int uvm_component::get_first_child(ref string name);
  return m_children.first(name);
endfunction

// UVM 1:1
function int uvm_component::get_num_children();
  return m_children.num();
endfunction

// UVM 1:1
function int uvm_component::has_child(string name);
  return m_children.exists(name);
endfunction

// UVM 1:1
function uvm_component uvm_component::lookup( string name );

  string leaf , remainder;
  uvm_component comp;
  uvm_root top;
  uvm_coreservice_t cs;
  cs = uvm_coreservice_t::get();
  top = cs.get_root();

  comp = this;

  m_extract_name(name, leaf, remainder);

  if (leaf == "") begin
    comp = top; // absolute lookup
    m_extract_name(remainder, leaf, remainder);
  end

  if (!comp.has_child(leaf)) begin
    `uvm_warning("Lookup Error",
       $sformatf("Cannot find child %0s",leaf))
    return null;
  end

  if( remainder != "" )
    return comp.m_children[leaf].lookup(remainder);

  return comp.m_children[leaf];

endfunction

// UVM 1:1
function int unsigned uvm_component::get_depth();
  if(m_name == "") return 0;
  get_depth = 1;
  foreach(m_name[i])
    if(m_name[i] == ".") ++get_depth;
endfunction

// UVM 1:1
function void uvm_component::m_extract_name(input string name ,
                                            output string leaf ,
                                            output string remainder );
  int i , len;
  len = name.len();

  for( i = 0; i < name.len(); i++ ) begin
    if( name[i] == "." ) begin
      break;
    end
  end

  if( i == len ) begin
    leaf = name;
    remainder = "";
    return;
  end

  leaf = name.substr( 0 , i - 1 );
  remainder = name.substr( i + 1 , len - 1 );

  return;
endfunction

// UVM 1:1
function bit uvm_component::m_add_child(uvm_component child);

  if (m_children.exists(child.get_name()) &&
      m_children[child.get_name()] != child) begin
      `uvm_warning("BDCLD",
        $sformatf("A child with the name '%0s' (type=%0s) already exists.",
           child.get_name(), m_children[child.get_name()].get_type_name()))
      return 0;
  end

  if (m_children_by_handle.exists(child)) begin
      `uvm_warning("BDCHLD",
        $sformatf("A child with the name '%0s' %0s %0s'",
                  child.get_name(),
                  "already exists in parent under name '",
                  m_children_by_handle[child].get_name()))
      return 0;
    end

  m_children[child.get_name()] = child;
  m_children_by_handle[child] = child;
  return 1;
endfunction

// UVM 1:1
function void uvm_component::set_name (string name);
  if(m_name != "") begin
    `uvm_error("INVSTNM", $sformatf("It is illegal to change the name of a component. The component name will not be changed to \"%s\"", name))
    return;
  end
  super.set_name(name);
  m_set_full_name();

endfunction

// UVM 1:1
function void uvm_component::m_set_full_name();
  uvm_root top;
  if ($cast(top, m_parent) || m_parent==null)
    m_name = get_name();
  else
    m_name = {m_parent.get_full_name(), ".", get_name()};

  foreach (m_children[c]) begin
    uvm_component tmp;
    tmp = m_children[c];
    tmp.m_set_full_name();
  end

endfunction

//-----
