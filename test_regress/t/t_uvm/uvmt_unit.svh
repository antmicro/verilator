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

`define UVMT_FAIL(THIS, FEATURE) begin \
  uvmt::test_failure_t failure = '{ FEATURE, `__FILE__, `__LINE__ }; \
  THIS.m_fail_feature(failure); \
end

`define UVMT_DONE(THIS, FEATURE) THIS.m_feature_done(FEATURE);

`define UVMT_RUN(TEST_CLASS) begin \
  TEST_CLASS test = new(); \
  test.run(); \
  if (test.get_status() != uvmt::S_PASS) $stop; \
end

`define UVMT_TOP(TEST_CLASS) \
module TEST_CLASS_top(); \
  initial begin \
    TEST_CLASS test = new(); \
    test.run(); \
    if (test.get_status() != uvmt::S_PASS) $stop; \
    $write("*-* All Finished *-*\n"); \
    $finish(); \
  end \
endmodule

`define UVMT_TEST_BEGIN(CLASS_NAME, TESTSUITE) \
class CLASS_NAME extends uvmt::uvm_test #(TESTSUITE, "CLASS_NAME"); \
  virtual task run_test(TESTSUITE ctx)
`define UVMT_TEST_END endtask endclass

`define UVMT_REGISTER_TEST(CLASS_NAME) begin \
  CLASS_NAME test = new(); \
  this.register_test(test); \
end

package uvmt;
  function void fatal(string msg);
    $display("[uvmt] FATAL: %s", msg);
    $stop;
  endfunction

  typedef enum bit [1:0] {
    S_PASS,
    S_FAIL,
    S_UNTESTED
  } feature_status_t;

  typedef struct {
    string feature;
    string file;
    int line;
  } test_failure_t;

  function string feature_status_name(feature_status_t status);
    case (status)
      S_PASS:     return "PASS";
      S_FAIL:     return "FAIL";
      S_UNTESTED: return "UNTESTED";
      default:    return "<UNKNOWN>";
    endcase
  endfunction

  typedef class uvm_testsuite;

  interface class uvm_test_base;
    pure virtual function string name();
    pure virtual task m_run_test(uvm_testsuite testsuite);
  endclass

  virtual class uvm_testsuite;
    function new(string name);
      m_name = name;
      m_status = S_UNTESTED;
    endfunction

    function void add_feature(string name);
      m_features[name] = S_UNTESTED;
    endfunction

    function void m_fail_feature(test_failure_t failure);
      m_mark_feature(failure.feature, S_FAIL);
      m_failures.push_back(failure);
    endfunction

    function void m_feature_done(string name);
      if (m_features[name] == S_FAIL) return;
      m_mark_feature(name, S_PASS);
    endfunction

    function void m_mark_feature(string name, feature_status_t status);
      if (!m_features.exists(name))
        fatal({"Feature ", name, " does not exist"});
      m_features[name] = status;
    endfunction

    function void register_test(uvm_test_base test);
      m_tests.push_back(test);
    endfunction

    string m_name;
    feature_status_t m_features[string];
    feature_status_t m_status;
    test_failure_t m_failures[$];
    uvm_test_base m_tests[$];


    virtual task setup(); endtask

    task run();
      int feature_count, features_passed, features_failed;

      feature_count = m_features.size();
      features_passed = 0;
      features_failed = 0;

      m_failures.delete();
      foreach (m_features[feature]) m_features[feature] = S_UNTESTED;

      foreach (m_tests[i]) begin
        uvm_test_base test = m_tests[i];
        setup();
        $display("[uvmt]: Running test \"%s\"", test.name());
        test.m_run_test(this);
      end

      $display("--- TEST SUITE SUMMARY: ---");
      foreach (m_features[feature]) begin
        feature_status_t status = m_features[feature];
        $display("  %s: ", feature, feature_status_name(status));
        if (status == S_PASS) features_passed++;
        if (status == S_FAIL) features_failed++;
      end
      $display("%0d/%0d features passed", features_passed,feature_count);
      if (features_failed == 0) begin
        $display("TEST SUITE \"%s\" PASSED!", m_name);
        m_status = S_PASS;
      end else begin
        $display("TEST SUITE \"%s\" FAILED!", m_name);
        m_status = S_UNTESTED;
      end

      if (m_failures.size() != 0) begin
        $display("FAILURES:");
        for (int i = 0; i < m_failures.size(); i++) begin
          test_failure_t failure = m_failures[i];
          $display("  %2d: \"%s\" at %s:%0d",
                   i + 1, failure.feature, failure.file, failure.line);
        end
      end

    endtask

    function feature_status_t get_status();
      return m_status;
    endfunction
  endclass

  virtual class uvm_test #(type TESTSUITE, string NAME) implements uvm_test_base;
    virtual function string name();
      return NAME;
    endfunction
    virtual task run_test(TESTSUITE ctx); endtask
    virtual task m_run_test(uvm_testsuite testsuite_base);
      TESTSUITE testsuite;
      if (!$cast(testsuite, testsuite_base))
        fatal($sformatf("Test %s does not belong to this suite", name()));
      run_test(testsuite);
    endtask
  endclass
endpackage

