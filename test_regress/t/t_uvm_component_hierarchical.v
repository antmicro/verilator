`include "t_uvm/uvmt_unit.svh"
`include "t_uvm/base/uvm_component.svh"

class my_uvm_component #(string S) extends uvm_component;
  function new(string name, uvm_component parent);
    super.new(name, parent);
  endfunction

  virtual function string get_type_name();
    return {"my_uvm_component_", S};
  endfunction
endclass

class uvm_component_testsuite extends uvmt::uvm_testsuite;
  uvm_root m_top_comp;
  my_uvm_component #("A") m_child1;
  my_uvm_component #("B") m_child2;

  extern function new();

  virtual task setup();
    uvm_component children[$];

    uvm_root::m_inst = null; // Drop the old root
    m_top_comp = uvm_coreservice_t::get().get_root();

    m_top_comp.get_children(children);
    if (children.size() != 0) `UVMT_FAIL(this, "get_children");

    m_child1 = new("child1", m_top_comp);
    m_child2 = new("child2", m_top_comp);

    `UVMT_DONE(this, "new");
  endtask
endclass

`UVMT_TEST_BEGIN(get_children_test, uvm_component_testsuite);
  uvm_component children[$];

  ctx.m_top_comp.get_children(children);
  if (children.size() != 2) `UVMT_FAIL(ctx, "get_children");

  foreach (children[i]) begin
    $display("child \"%s\" of type %s", children[i].get_full_name(),
              children[i].get_type_name());
  end
  `UVMT_DONE(ctx, "get_children");
`UVMT_TEST_END

`UVMT_TEST_BEGIN(get_parent_test, uvm_component_testsuite);
  if (ctx.m_child1.get_parent() != ctx.m_top_comp) `UVMT_FAIL(ctx, "get_parent");
  if (ctx.m_child2.get_parent() != ctx.m_top_comp) `UVMT_FAIL(ctx, "get_parent");
  `UVMT_DONE(ctx, "get_parent");
`UVMT_TEST_END

`UVMT_TEST_BEGIN(get_child_test, uvm_component_testsuite);
  if (ctx.m_top_comp.get_child("child1") != ctx.m_child1) `UVMT_FAIL(ctx, "get_child");
  if (ctx.m_top_comp.get_child("child2") != ctx.m_child2) `UVMT_FAIL(ctx, "get_child");
  `UVMT_DONE(ctx, "get_child");
`UVMT_TEST_END

`UVMT_TEST_BEGIN(get_next_child_test, uvm_component_testsuite);
  string next_child = "child1";
  if (!ctx.m_top_comp.get_next_child(next_child)) `UVMT_FAIL(ctx, "get_next_child");
  if (next_child != "child2") `UVMT_FAIL(ctx, "get_next_child");
  if (ctx.m_top_comp.get_next_child(next_child)) `UVMT_FAIL(ctx, "get_next_child");
  `UVMT_DONE(ctx, "get_next_child");
`UVMT_TEST_END

`UVMT_TEST_BEGIN(get_first_child_test, uvm_component_testsuite);
  string next_child = "child1";
  if (!ctx.m_top_comp.get_next_child(next_child)) `UVMT_FAIL(ctx, "get_next_child");
  if (next_child != "child2") `UVMT_FAIL(ctx, "get_next_child");
  if (ctx.m_top_comp.get_next_child(next_child)) `UVMT_FAIL(ctx, "get_next_child");
  `UVMT_DONE(ctx, "get_next_child");
`UVMT_TEST_END

`UVMT_TEST_BEGIN(get_full_name_test, uvm_component_testsuite);
  my_uvm_component #("C") new_child;
  new_child = new("child3", ctx.m_child2);
  if (ctx.m_top_comp.get_full_name() != "") `UVMT_FAIL(ctx, "get_full_name");
  if (ctx.m_child1.get_full_name() != "child1") `UVMT_FAIL(ctx, "get_full_name");
  if (ctx.m_child2.get_full_name() != "child2") `UVMT_FAIL(ctx, "get_full_name");
  if (new_child.get_full_name() != "child2.child3") `UVMT_FAIL(ctx, "get_full_name");
  `UVMT_DONE(ctx, "get_full_name");
`UVMT_TEST_END

`UVMT_TEST_BEGIN(get_num_children_test, uvm_component_testsuite);
  my_uvm_component #("C") new_child;
  if (ctx.m_top_comp.get_num_children() != 2) `UVMT_FAIL(ctx, "get_num_children");
  new_child = new("child3", ctx.m_top_comp);
  if (ctx.m_top_comp.get_num_children() != 3) `UVMT_FAIL(ctx, "get_num_children");
  `UVMT_DONE(ctx, "get_num_children");
`UVMT_TEST_END

`UVMT_TEST_BEGIN(has_child_test, uvm_component_testsuite);
  my_uvm_component #("C") new_child;
  if (ctx.m_top_comp.has_child("nonexistent-child")) `UVMT_FAIL(ctx, "has_child");
  if (!ctx.m_top_comp.has_child("child1")) `UVMT_FAIL(ctx, "has_child");
  if (!ctx.m_top_comp.has_child("child2")) `UVMT_FAIL(ctx, "has_child");
  if (ctx.m_top_comp.has_child("child3")) `UVMT_FAIL(ctx, "has_child");
  new_child = new("child3", ctx.m_top_comp);
  if (!ctx.m_top_comp.has_child("child3")) `UVMT_FAIL(ctx, "has_child");
  `UVMT_DONE(ctx, "has_child");
`UVMT_TEST_END

`UVMT_TEST_BEGIN(get_depth_test, uvm_component_testsuite);
  my_uvm_component #("C") new_child;
  new_child = new("child3", ctx.m_child2);

  if (ctx.m_top_comp.get_depth() != 0) `UVMT_FAIL(ctx, "get_depth");
  if (ctx.m_child1.get_depth() != 1) `UVMT_FAIL(ctx, "get_depth");
  if (ctx.m_child2.get_depth() != 1) `UVMT_FAIL(ctx, "get_depth");
  if (new_child.get_depth() != 2) `UVMT_FAIL(ctx, "get_depth");
  `UVMT_DONE(ctx, "get_depth");
`UVMT_TEST_END

`UVMT_TEST_BEGIN(lookup_test, uvm_component_testsuite);
  my_uvm_component #("C") new_child;
  new_child = new("child3", ctx.m_child2);


  if (ctx.m_top_comp.lookup("child1") != ctx.m_child1) `UVMT_FAIL(ctx, "lookup");
  if (ctx.m_top_comp.lookup("child2") != ctx.m_child2) `UVMT_FAIL(ctx, "lookup");
  if (ctx.m_top_comp.lookup("child2.child3") != new_child) `UVMT_FAIL(ctx, "lookup");
  `UVMT_DONE(ctx, "lookup");
`UVMT_TEST_END

function uvm_component_testsuite::new();
  super.new("uvm_component hierarchical");

  add_feature("new");               /* Tested during set-up phase */
  add_feature("get_parent");        `UVMT_REGISTER_TEST(get_parent_test);
  add_feature("get_full_name");     `UVMT_REGISTER_TEST(get_full_name_test);
  add_feature("get_children");      `UVMT_REGISTER_TEST(get_children_test);
  add_feature("get_child");         `UVMT_REGISTER_TEST(get_child_test);
  add_feature("get_next_child");    `UVMT_REGISTER_TEST(get_next_child_test);
  add_feature("get_num_children");  `UVMT_REGISTER_TEST(get_num_children_test);
  add_feature("has_child");         `UVMT_REGISTER_TEST(has_child_test);
  add_feature("lookup");            `UVMT_REGISTER_TEST(lookup_test);
  add_feature("get_depth");         `UVMT_REGISTER_TEST(get_depth_test);
endfunction

`UVMT_TOP(uvm_component_testsuite)
