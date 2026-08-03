// Extracted from OpenTitan:
//   hw/ip/usbdev/dv/env/usbdev_env_cfg.sv
// Variables:
//   aon_clk_freq_khz : int unsigned (32-bit) -- AON clock frequency in kHz
//
// Constraint:
//   aon_clk_freq_khz > 48_000 / 300   (i.e. > 160, so >= 161)
//   aon_clk_freq_khz <= 48_000 / 48   (i.e. <= 1000)
//
// Solutions: {161, 162, ..., 1000} = 840 solutions

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
