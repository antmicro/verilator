// Real-world benchmark extracted from OpenTitan spi_device_base_vseq.sv
// Source: hw/ip/spi_device/dv/env/seq_lib/spi_device_base_vseq.sv, lines 24-33
//
// Variables:
//   spi_freq_faster   : bit          -- whether SPI clock is faster than core clock
//   core_spi_freq_ratio : bit [3:0]  -- ratio of core to SPI clock frequency, in [1:8]
//
// Constraint:
//   ratio inside {[1:8]}
//   if spi_freq_faster, then ratio must be <= 4
//   (faster SPI requires a tighter clock ratio)
//
// Solutions: 12 total
//   spi_freq_faster=0: ratio in {1,2,3,4,5,6,7,8}  -- 8 solutions
//   spi_freq_faster=1: ratio in {1,2,3,4}           -- 4 solutions
//
// Canonical order: sorted by (spi_freq_faster, core_spi_freq_ratio)

`ifndef NUM_ITERATIONS
  `define NUM_ITERATIONS 10000
`endif

module distr_ot_spi_freq;
  class C;
    rand bit        spi_freq_faster;
    rand bit [3:0]  core_spi_freq_ratio;

    constraint freq_c {
      core_spi_freq_ratio inside {[1:8]};
      spi_freq_faster -> core_spi_freq_ratio <= 4;
    }
  endclass

  initial begin
    automatic C c = new;
    repeat (`NUM_ITERATIONS) begin
      if (!bit'(c.randomize())) $display("-UNSAT");
      else $display("%0d %0d", c.spi_freq_faster, c.core_spi_freq_ratio);
    end
    $finish;
  end
endmodule
