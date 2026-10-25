// Specify block: path declarations and system timing checks, A.7.
module specify_demo(input a, input b, output c);
  specparam tRise = 2, tFall = 3;

  specify
    (a => c) = (tRise, tFall);
    if (b) (a *> c) = 4;
    $setup(a, posedge b, 2);
    $width(edge[01, 10] b, 3);
    $hold(posedge a, b, 1);
  endspecify

  assign c = a & b;
endmodule
