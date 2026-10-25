// Gate and switch primitives (IEEE 1364-2005 A.3).
module gates(input a, input b, output y1, output y2, output y3, output y4);
  wire w;

  and (y1, a, b);
  nand #1 (y2, a, b);
  not (y3, w);
  bufif0 (w, a, b);
  pullup (w);
  pulldown (strong0) (y4);
  nmos (w, a, b);
  cmos (w, a, b, b);
  tran (a, b);
  tranif1 (a, b, w);
endmodule
