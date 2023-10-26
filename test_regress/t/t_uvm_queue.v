// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain, for
// any use, without warranty, 2023 by Antmicro Ltd.
// SPDX-License-Identifier: CC0-1.0

`include "uvmt_unit.svh"
`include "uvmt_logs.svh"

`include "base/uvm_object.svh"
`include "base/uvm_queue.svh"

typedef uvm_queue #(string) strq_t;
typedef uvm_queue #(int) intq_t;

class uvm_queue_testsuite extends uvmt::uvm_testsuite;
  strq_t m_strq;
  intq_t m_intq;

  extern function new();

  virtual task setup();
    strq_t::uvmt_drop_globals();
    intq_t::uvmt_drop_globals();

    m_strq = new("strq");
    m_intq = new("intq");
    `UVMT_DONE(this, "new");
  endtask
endclass

`UVMT_TEST_BEGIN(size_test, uvm_queue_testsuite);
  if (ctx.m_strq.size() != 0) `UVMT_FAIL(ctx, "size");
  if (ctx.m_intq.size() != 0) `UVMT_FAIL(ctx, "size");
  ctx.m_strq.push_back("a");
  if (ctx.m_strq.size() != 1) `UVMT_FAIL(ctx, "size");
  ctx.m_strq.push_back("a");
  if (ctx.m_strq.size() != 2) `UVMT_FAIL(ctx, "size");
  `UVMT_DONE(ctx, "size");
`UVMT_TEST_END

`UVMT_TEST_BEGIN(push_pop_test, uvm_queue_testsuite);
  ctx.m_strq.push_back("a");
  ctx.m_strq.push_back("b");
  ctx.m_strq.push_back("c");
  ctx.m_strq.push_back("d");
  if (ctx.m_strq.size() != 4) `UVMT_FAIL(ctx, "push_back");
  `UVMT_DONE(ctx, "push_back");
  if (ctx.m_strq.pop_front() != "a") `UVMT_FAIL(ctx, "pop_front");
  if (ctx.m_strq.pop_front() != "b") `UVMT_FAIL(ctx, "pop_front");
  if (ctx.m_strq.pop_front() != "c") `UVMT_FAIL(ctx, "pop_front");
  if (ctx.m_strq.pop_front() != "d") `UVMT_FAIL(ctx, "pop_front");
  `UVMT_DONE(ctx, "pop_front");

  ctx.m_intq.push_front(1);
  ctx.m_intq.push_front(2);
  ctx.m_intq.push_front(3);
  ctx.m_intq.push_front(4);
  if (ctx.m_intq.size() != 4) `UVMT_FAIL(ctx, "push_front");
  `UVMT_DONE(ctx, "push_front");
  if (ctx.m_intq.pop_back() != 1) `UVMT_FAIL(ctx, "pop_back");
  if (ctx.m_intq.pop_back() != 2) `UVMT_FAIL(ctx, "pop_back");
  if (ctx.m_intq.pop_back() != 3) `UVMT_FAIL(ctx, "pop_back");
  if (ctx.m_intq.pop_back() != 4) `UVMT_FAIL(ctx, "pop_back");
  `UVMT_DONE(ctx, "pop_back");
`UVMT_TEST_END

`UVMT_TEST_BEGIN(global_queue_test, uvm_queue_testsuite);
  strq_t global_strq;
  intq_t global_intq;
  global_strq = strq_t::get_global_queue();
  global_intq = intq_t::get_global_queue();
  if (global_strq == null) `UVMT_FAIL(ctx, "get_global_queue");
  if (global_intq == null) `UVMT_FAIL(ctx, "get_global_queue");
  `UVMT_DONE(ctx, "get_global_queue");

  global_strq.push_back("foo");
  if (strq_t::get_global(0) != "foo") `UVMT_FAIL(ctx, "get_global");

  global_intq.push_back(2138);
  if (intq_t::get_global(0) != 2138) `UVMT_FAIL(ctx, "get_global");
  `UVMT_DONE(ctx, "get_global");
`UVMT_TEST_END

class insert_get_test extends uvmt::uvm_test #(uvm_queue_testsuite, "insert_get_test");
  function string null_name(int idx);
    return $sformatf("null%0d", idx);
  endfunction

  virtual task run_test(uvm_queue_testsuite ctx);
    for (int i = 0; i < 10; i++) ctx.m_strq.push_back(null_name(i));
    for (int i = 0; i < 10; i++) begin
      if (ctx.m_strq.get(i) != null_name(i)) `UVMT_FAIL(ctx, "get");
    end
    $display("queue size: %0d", ctx.m_strq.size());
    //ctx.m_strq.insert(10, "last"); - uvm_queue has a faulty bound check and disallows
    //                                 insert at >= size() instead of > size().
    //                                 SystemVerilog queues should allow insert at size()
    ctx.m_strq.insert(5, "sixth");
    ctx.m_strq.insert(0, "first");
    //if (ctx.m_strq.size() != 13) begin
    if (ctx.m_strq.size() != 12) begin
      $display("queue size: %0d", ctx.m_strq.size());
      `UVMT_FAIL(ctx, "insert");
    end
    if (ctx.m_strq.get(0) != "first") `UVMT_FAIL(ctx, "get");
    //if (ctx.m_strq.get(12) != "last") `UVMT_FAIL(ctx, "get");
    if (ctx.m_strq.get(6) != "sixth") `UVMT_FAIL(ctx, "get");
    `UVMT_DONE(ctx, "insert");
    `UVMT_DONE(ctx, "get");
  endtask
endclass

`UVMT_TEST_BEGIN(delete_test, uvm_queue_testsuite);
  for (int i = 0; i < 10; i++) ctx.m_intq.push_back(i);
  ctx.m_intq.delete(5);
  if (ctx.m_intq.size() != 9) `UVMT_FAIL(ctx, "delete");
  if (ctx.m_intq.get(5) != 6) `UVMT_FAIL(ctx, "delete");
  if (ctx.m_intq.get(4) != 4) `UVMT_FAIL(ctx, "delete");
  ctx.m_intq.delete();
  if (ctx.m_intq.size() != 0) `UVMT_FAIL(ctx, "delete");
  `UVMT_DONE(ctx, "delete");
`UVMT_TEST_END

`UVMT_TEST_BEGIN(wait_until_not_empty_test, uvm_queue_testsuite);
  fork begin
    #10 ctx.m_intq.push_back(1);
  end join_none

  if (ctx.m_intq.size() != 0) `UVMT_FAIL(ctx, "wait_until_not_empty");
  $display("Waiting for non-empty queue");
  ctx.m_intq.wait_until_not_empty();
  if (ctx.m_intq.size() != 1) `UVMT_FAIL(ctx, "wait_until_not_empty");
  `UVMT_DONE(ctx, "wait_until_not_empty");
`UVMT_TEST_END

function uvm_queue_testsuite::new();
  super.new("uvm_queue");

  add_feature("new");                  /* Tested during set-up phase */
  add_feature("size");                 `UVMT_REGISTER_TEST(size_test);
  add_feature("push_back");            `UVMT_REGISTER_TEST(push_pop_test);
  add_feature("pop_front");
  add_feature("push_front");
  add_feature("pop_back");
  add_feature("get");                  `UVMT_REGISTER_TEST(insert_get_test);
  add_feature("insert");
  add_feature("delete");               `UVMT_REGISTER_TEST(delete_test);
  add_feature("get_global_queue");     `UVMT_REGISTER_TEST(global_queue_test);
  add_feature("get_global");
  add_feature("wait_until_not_empty"); `UVMT_REGISTER_TEST(wait_until_not_empty_test);
endfunction

`UVMT_TOP(uvm_queue_testsuite)
