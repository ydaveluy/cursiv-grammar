// Conditionals nested in either order: `ifdef and `ifndef deepen one count,
// so the text up to the `endif that balances the outer one stays hidden.
module guarded;
`ifdef FOO
`ifndef BAR
  not verilog at all
`endif
  still hidden (
`endif
`ifndef BAZ
  `ifdef QUX { `endif
  ) hidden
`endif
  wire w;
endmodule
