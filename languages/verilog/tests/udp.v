// UDP declaration (IEEE 1364-2005 A.5), combinational and sequential.
primitive udp_mux(out, sel, a, b);
  output out;
  input sel, a, b;

  table
    0 0 ? : 0;
    0 1 ? : 1;
    1 ? 0 : 0;
    1 ? 1 : 1;
  endtable
endprimitive

primitive udp_latch(q, clk, d);
  output q;
  reg q;
  input clk, d;

  initial q = 1'b0;

  table
    (01) 0 : ? : 0;
    (01) 1 : ? : 1;
    (0x) 1 : 1 : 1;
    (0x) 0 : 0 : 0;
    (?0) ? : ? : -;
    0 ? : ? : -;
  endtable
endprimitive
