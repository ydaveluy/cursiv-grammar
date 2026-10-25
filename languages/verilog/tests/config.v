// Configuration source text (IEEE 1364-2005 A.1.5): reached from the
// top-level grammar directly; the separate library-source form is not
// ported.
config cfg1;
  design work.top;
  default liblist work fast;
  instance top.u1 use work.slow_cell : config;
  cell work.adder liblist work fast;
endconfig
