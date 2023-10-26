// Copyright 2023 by Antmicro Ltd.
// SPDX-License-Identifier: Apache-2.0
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

typedef enum {
  UVM_NONE   = 0,
  UVM_LOW    = 100,
  UVM_MEDIUM = 200,
  UVM_HIGH   = 300,
  UVM_FULL   = 400,
  UVM_DEBUG  = 500
} uvm_verbosity;

typedef enum bit [1:0] {
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

function void uvm_report_fatal(string id,
	                             string message,
                               int verbosity = UVM_NONE,
			                         string filename = "",
			                         int line = 0,
                               string context_name = "",
                               bit report_enabled_checked = 0);
  $display("(mock) {UVM} FATAL ERROR: [%s] %s ", id, message);
  if (filename != "")
    $display("(mock)              ^ at %s:%0d ", filename, line);
  $stop();
endfunction

function void uvm_report_error(string id,
                               string message,
                               int verbosity = UVM_NONE,
			                         string filename = "",
			                         int line = 0,
                               string context_name = "",
                               bit report_enabled_checked = 0);
  $display("(mock) {UVM} ERROR: [%s] %s ", id, message);
  if (filename != "")
    $display("(mock)              ^ at %s:%0d ", filename, line);
endfunction

function void uvm_report_warning(string id,
                                 string message,
                                 int verbosity = UVM_MEDIUM,
                                 string filename = "",
                                 int line = 0,
                                 string context_name = "",
                                 bit report_enabled_checked = 0);
  $display("(mock) {UVM} WARNING: [%s] %s ", id, message);
  if (filename != "")
    $display("(mock)              ^ at %s:%0d ", filename, line);
endfunction
