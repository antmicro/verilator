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

// Status: MOCK

class uvm_object;
  local string m_leaf_name;
  local int m_inst_id;
  static protected int m_inst_count;

  extern function new(string name="");
  extern function string get_name();
  extern function void set_name(string name);
  extern function int get_inst_id();

  virtual function string get_type_name (); return "<unknown>"; endfunction
endclass

// UVM 1:1*
function uvm_object::new(string name="");
  //$display("[mock] Created an object with name %s", name);
  m_inst_id = m_inst_count++;
  m_leaf_name = name;
endfunction

// UVM 1:1
function string uvm_object::get_name ();
  return m_leaf_name;
endfunction

// UVM 1:1
function void uvm_object::set_name (string name);
  m_leaf_name = name;
endfunction

// UVM 1:1
function int uvm_object::get_inst_id();
  return m_inst_id;
endfunction
