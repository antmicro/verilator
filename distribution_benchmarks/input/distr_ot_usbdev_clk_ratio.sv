// Extracted from OpenTitan:
//   hw/ip/usbdev/dv/env/usbdev_env_cfg.sv
//
// Models the AON clock frequency constraints for the USB device:
//   - USB clock is fixed at 48 MHz (48_000 kHz)
//   - AON clock must be strictly slower than USB/48 (~1 MHz upper bound)
//     and strictly faster than USB/300 (~160 kHz lower bound)
//   Original rationale: the aon_wake logic requires AON < 1 MHz to avoid
//   spurious bus-reset reports when detecting Low Speed signaling (2 bit
//   intervals @ 1.5 Mbps can look high for 3 edges above 1 MHz).
//
// Variables:
//   aon_clk_freq_khz : int unsigned (32-bit) -- AON clock frequency in kHz
//
// Constraint:
//   aon_clk_freq_khz > 48_000 / 300   (i.e. > 160, so >= 161)
//   aon_clk_freq_khz <= 48_000 / 48   (i.e. <= 1000)
//
// Solutions: {161, 162, ..., 1000} = 840 solutions
//
// Note: although expressed as arithmetic on a constant, Verilator folds
// 48_000/300 and 48_000/48 at compile time, so the SMT-LIB encoding is
// equivalent to a plain range membership constraint (bvugt + bvule).
// This benchmark is most useful for Dim5 (32-bit variable, medium |M|).
//
// Canonical output order: ascending integer value of aon_clk_freq_khz

`ifndef NUM_ITERATIONS
  `define NUM_ITERATIONS 10000
`endif

module distr_ot_usbdev_clk_ratio;
  localparam int unsigned USB_CLK_FREQ_KHZ = 48_000;

  class C;
    rand int unsigned aon_clk_freq_khz;

    constraint aon_clk_freq_khz_c {
      aon_clk_freq_khz >  USB_CLK_FREQ_KHZ / 300 &&
      aon_clk_freq_khz <= USB_CLK_FREQ_KHZ / 48;
    }
  endclass

  initial begin
    automatic C c = new;
    repeat (`NUM_ITERATIONS) begin
      if (!bit'(c.randomize())) $display("-UNSAT");
      else $display("%0d", c.aon_clk_freq_khz);
    end
    $finish;
  end
endmodule
