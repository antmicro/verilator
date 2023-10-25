// SPDX-License-Identifier: Apache-2.0
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

//UVM virtual class uvm_coreservice_t;
class uvm_coreservice_t;
//UVM
  `ifdef VERILATOR
	static uvm_coreservice_t inst;
`else
	local static uvm_coreservice_t inst;
`endif

  // UVM ~
  static function uvm_coreservice_t get();
  //UVM  if(inst==null)
  //UVM  uvm_init(null);
    if(inst==null)
      inst = new;
  //UVM

    return inst;
  endfunction // get

  // UVM 1:1
  virtual function uvm_root get_root();
    return uvm_root::m_uvm_get_root();
  endfunction
endclass
