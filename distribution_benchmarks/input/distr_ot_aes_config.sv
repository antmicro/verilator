// Variables:
//   has_error   : bit          -- 0 = normal config, 1 = injected config error
//   op          : bit [1:0]    -- AES operation (ENC=2'b01, DEC=2'b10)
//   key_len     : bit [2:0]    -- key length (AES-128=3'b001, -192=3'b010, -256=3'b100)
//   mode        : bit [5:0]    -- cipher mode (one-hot: ECB/CBC/CFB/OFB/CTR/GCM; NONE=6'b111111)
//   reseed_rate : bit [2:0]    -- PRNG reseed rate (PER_1=3'b001, PER_64=3'b010, PER_8K=3'b100)
//
// Encoding constants from aes_pkg.sv:
//   AES_ENC  = 2'b01,  AES_DEC  = 2'b10
//   AES_128  = 3'b001, AES_192  = 3'b010, AES_256  = 3'b100
//   AES_ECB  = 6'b00_0001, AES_CBC  = 6'b00_0010, AES_CFB  = 6'b00_0100,
//   AES_OFB  = 6'b00_1000, AES_CTR  = 6'b01_0000, AES_GCM  = 6'b10_0000,
//   AES_NONE = 6'b11_1111  (illegal sentinel, used as the error-mode target)
//   PER_1    = 3'b001, PER_64   = 3'b010, PER_8K   = 3'b100
//
// Constraints (simplified from aes_message_item; dist weights removed):
//   has_error == 0 ->
//     op          inside {AES_ENC, AES_DEC}
//     key_len     inside {AES_128, AES_192, AES_256}
//     mode        inside {AES_ECB, AES_CBC, AES_CFB, AES_OFB, AES_CTR, AES_GCM}
//     reseed_rate inside {PER_1, PER_64, PER_8K}
//   has_error == 1 ->
//     op          inside {2'b00, 2'b11}           (complement of valid ops)
//     key_len     inside {3'b000,3'b011,3'b101,3'b110,3'b111}  (complement of valid keys)
//     mode        == AES_NONE                      (specific illegal sentinel)
//     reseed_rate inside {3'b000,3'b011,3'b101,3'b110,3'b111}  (complement of valid rates)
//
// Solutions: 158 total
//   has_error=0: 2 ops x 3 key_lens x 6 modes x 3 reseed_rates = 108
//   has_error=1: 2 ops x 5 key_lens x 1 mode x 5 reseed_rates  =  50


`ifndef NUM_ITERATIONS
  `define NUM_ITERATIONS 10000
`endif

module distr_ot_aes_config;
  // Encoding constants from aes_pkg.sv
  localparam bit [1:0] AES_ENC  = 2'b01;
  localparam bit [1:0] AES_DEC  = 2'b10;

  localparam bit [2:0] AES_128  = 3'b001;
  localparam bit [2:0] AES_192  = 3'b010;
  localparam bit [2:0] AES_256  = 3'b100;

  localparam bit [5:0] AES_ECB  = 6'b00_0001;
  localparam bit [5:0] AES_CBC  = 6'b00_0010;
  localparam bit [5:0] AES_CFB  = 6'b00_0100;
  localparam bit [5:0] AES_OFB  = 6'b00_1000;
  localparam bit [5:0] AES_CTR  = 6'b01_0000;
  localparam bit [5:0] AES_GCM  = 6'b10_0000;
  localparam bit [5:0] AES_NONE = 6'b11_1111;

  localparam bit [2:0] PER_1    = 3'b001;
  localparam bit [2:0] PER_64   = 3'b010;
  localparam bit [2:0] PER_8K   = 3'b100;

  class C;
    rand bit        has_error;
    rand bit [1:0]  op;
    rand bit [2:0]  key_len;
    rand bit [5:0]  mode;
    rand bit [2:0]  reseed_rate;

    constraint aes_config_c {
      if (!has_error) {
        op          inside {AES_ENC, AES_DEC};
        key_len     inside {AES_128, AES_192, AES_256};
        mode        inside {AES_ECB, AES_CBC, AES_CFB, AES_OFB, AES_CTR, AES_GCM};
        reseed_rate inside {PER_1, PER_64, PER_8K};
      } else {
        op          inside {2'b00, 2'b11};
        key_len     inside {3'b000, 3'b011, 3'b101, 3'b110, 3'b111};
        mode        == AES_NONE;
        reseed_rate inside {3'b000, 3'b011, 3'b101, 3'b110, 3'b111};
      }
    }
  endclass

  initial begin
    automatic C c = new;
    repeat (`NUM_ITERATIONS) begin
      if (!bit'(c.randomize())) $display("-UNSAT");
      else $display("%0d %0d %0d %0d %0d",
               c.has_error, c.op, c.key_len, c.mode, c.reseed_rate);
    end
    $finish;
  end
endmodule
