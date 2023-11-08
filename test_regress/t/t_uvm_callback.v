// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain, for
// any use, without warranty, 2023 by Antmicro Ltd.
// SPDX-License-Identifier: CC0-1.0

`include "uvmt_unit.svh"
`include "uvm_pkg.sv"
`include "uvmt_extras.svh"

import uvm_pkg::*;

typedef class test_cb;

class cb_ctx extends uvm_object;
  `uvm_register_cb(cb_ctx, test_cb)
  `uvmt_register_cb(cb_ctx, test_cb)

  string m_strq[$];

  function new(string name); super.new(name); endfunction

  function void append_str(string str);
    m_strq.push_back(str);
  endfunction
endclass

class test_cb extends uvm_callback;
  function new(string name); super.new(name); endfunction

  function void append_str(cb_ctx ctx);
    ctx.append_str(get_name());
  endfunction
endclass

class uvm_callback_testsuite extends uvmt::uvm_testsuite;
  cb_ctx m_ctx[string];

  extern function new();

  virtual task setup();
    cb_ctx::uvmt_drop_and_reregister_cb();
    m_ctx.delete();
    m_ctx["foo"] = new("foo");
    m_ctx["bar"] = new("bar");
  endtask

  function void add_callback(string ctx_name, test_cb cb);
    uvm_callbacks#(cb_ctx, test_cb)::add(m_ctx[ctx_name], cb);
  endfunction

  function void delete_callback(string ctx_name, test_cb cb);
    uvm_callbacks#(cb_ctx, test_cb)::delete(m_ctx[ctx_name], cb);
  endfunction


  function void do_callbacks(string ctx_name);
    `uvm_do_obj_callbacks(cb_ctx, test_cb, m_ctx[ctx_name],
                          append_str(m_ctx[ctx_name]))
  endfunction

  function bit ctx_queue_match(string ctx_name, string q[$]);
    cb_ctx ctx = m_ctx[ctx_name];
    if (ctx.m_strq.size() != q.size()) return 0;
    foreach (ctx.m_strq[i]) begin
      if (ctx.m_strq[i] != q[i]) return 0;
    end
    return 1;
  endfunction

  function void ctx_clear(string ctx_name);
    m_ctx[ctx_name].m_strq.delete();
  endfunction
endclass

`UVMT_TEST_BEGIN(callback_test, uvm_callback_testsuite);
  string q_expect[$];
  test_cb callback_1, callback_2;

  callback_1 = new("callback_1");
  callback_2 = new("callback_2");

  ctx.add_callback("foo", callback_1);
  ctx.do_callbacks("foo");
  ctx.do_callbacks("bar");
  //$display("%p", ctx.m_ctx["foo"].m_strq);

  q_expect = '{"callback_1"};
  if (!ctx.ctx_queue_match("foo", q_expect)) `UVMT_FAIL(ctx, "add");
  q_expect = '{};
  if (!ctx.ctx_queue_match("bar", q_expect)) `UVMT_FAIL(ctx, "add");

  ctx.add_callback("foo", callback_2);
  ctx.add_callback("bar", callback_2);
  ctx.do_callbacks("foo");
  ctx.do_callbacks("bar");

  q_expect = '{"callback_1", "callback_1", "callback_2"};
  if (!ctx.ctx_queue_match("foo", q_expect)) `UVMT_FAIL(ctx, "add");
  q_expect = '{"callback_2"};
  if (!ctx.ctx_queue_match("bar", q_expect)) `UVMT_FAIL(ctx, "add");

  `UVMT_DONE(ctx, "add");
`UVMT_TEST_END

`UVMT_TEST_BEGIN(delete_test, uvm_callback_testsuite);
  string q_expect[$];
  test_cb callback_1, callback_2;

  callback_1 = new("callback_1");
  callback_2 = new("callback_2");

  ctx.add_callback("foo", callback_1);
  ctx.add_callback("foo", callback_2);
  ctx.do_callbacks("foo");
  q_expect = '{"callback_1", "callback_2"};
  if (!ctx.ctx_queue_match("foo", q_expect)) `UVMT_FAIL(ctx, "add");

  ctx.delete_callback("foo", callback_1);
  ctx.do_callbacks("foo");
  q_expect = '{"callback_1", "callback_2", "callback_2"};
  if (!ctx.ctx_queue_match("foo", q_expect)) `UVMT_FAIL(ctx, "delete");

  ctx.delete_callback("foo", callback_2);
  ctx.do_callbacks("foo");
  q_expect = '{"callback_1", "callback_2", "callback_2"};
  if (!ctx.ctx_queue_match("foo", q_expect)) `UVMT_FAIL(ctx, "delete");

  `UVMT_DONE(ctx, "delete");
`UVMT_TEST_END

function uvm_callback_testsuite::new();
  super.new("uvm_callback");

  add_feature("add");            `UVMT_REGISTER_TEST(callback_test);
  add_feature("delete");         `UVMT_REGISTER_TEST(delete_test);
  add_feature("add_by_name",     uvmt::S_DISABLED);
  add_feature("delete_by_name",  uvmt::S_DISABLED);
endfunction

`UVMT_TOP(uvm_callback_testsuite)
