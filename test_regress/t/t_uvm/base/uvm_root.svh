//
//------------------------------------------------------------------------------
// Copyright 2007-2011 Mentor Graphics Corporation
// Copyright 2014 Semifore
// Copyright 2010-2018 Synopsys, Inc.
// Copyright 2007-2018 Cadence Design Systems, Inc.
// Copyright 2010-2012 AMD
// Copyright 2012-2018 NVIDIA Corporation
// Copyright 2012-2018 Cisco Systems, Inc.
// Copyright 2012 Accellera Systems Initiative
// Copyright 2017 Verific
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

class uvm_root extends uvm_component;
  //UVM static local uvm_root m_inst;
  static uvm_root m_inst;
  //UVM

  extern protected function new ();
  extern static function uvm_root m_uvm_get_root();
endclass

// UVM ~
function uvm_root::new();
  //UVM uvm_report_handler rh;
  super.new("__top__", null);

  // For error reporting purposes, we need to construct this first.
  //UVM rh = new("reporter");
  //UVM set_report_handler(rh);

  // Checking/Setting this here makes it much harder to
  // trick uvm_init into infinite recursions
  if (m_inst != null) begin
  //UVM  `uvm_fatal_context("UVM/ROOT/MULTI",
  //UVM                     "Attempting to construct multiple roots",
  //UVM                     m_inst)
  $display("UVM/ROOT/MULTI: Attempting to construct multiple roots");
  //UVM
    return;
  end
  m_inst = this;
`ifdef UVM_ENABLE_DEPRECATED_API
   uvm_top = this; 
`endif

  //UVM clp = uvm_cmdline_processor::get_inst();

endfunction

// UVM ~
function uvm_root uvm_root::m_uvm_get_root();
  if (m_inst == null) begin
    uvm_root top;
    top = new();

    if (top != m_inst)
      // Something very, very bad has happened and
      // we already fatal'd.  Throw out the garbage
      // root.
      return null;

  //UVM  top.m_domain = uvm_domain::get_uvm_domain();
  end
  return m_inst;
endfunction
