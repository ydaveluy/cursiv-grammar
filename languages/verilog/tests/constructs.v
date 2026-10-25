// Constructs the corpus modules do not exercise: escaped identifiers, real
// and based numbers, generate case, fork/join, functions and tasks,
// attributes, event control, defparam, indexed and part selects.
module constructs;
  parameter WIDTH = 8;
  genvar i;
  integer \my-counter ;

  real r1;
  initial begin
    r1 = 3.25;
    r1 = 1.5e-3;
  end

  reg [7:0] byte_a;
  reg [3:0] nib_a, nib_b;
  initial begin
    byte_a = 8'hFF;
    byte_a = 4'sb1010;
    byte_a = 'o17;
    byte_a = 'd9;
    nib_a = byte_a[7:4];
    nib_b = byte_a[3-:2];
  end

  generate
    for (i = 0; i < 4; i = i + 1) begin : gen_loop
      case (i)
        0: wire w0 = 1'b0;
        1, 2: wire w12 = 1'b1;
        default: wire wd = 1'bx;
      endcase
    end
  endgenerate

  event done_evt;
  initial begin : trigger_block
    -> done_evt;
    disable trigger_block;
  end

  function [7:0] add_one;
    input [7:0] value;
    begin
      add_one = value + 1;
    end
  endfunction

  task automatic wait_and_set(input [7:0] value, output [7:0] result);
    begin
      wait (value != 0);
      result = add_one(value);
    end
  endtask

  wire [7:0] result;
  reg [7:0] source;
  (* full_case, parallel_case *)
  always @(posedge byte_a[0]) fork
    source = add_one(byte_a);
  join

  sub #(.WIDTH(WIDTH)) inst1 (.a(byte_a), .b(result));
  defparam inst1.WIDTH = 4;
endmodule

module sub #(parameter WIDTH = 1) (input [WIDTH-1:0] a, output [WIDTH-1:0] b);
  assign b = a;
endmodule
