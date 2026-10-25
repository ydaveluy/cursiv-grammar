context ctx1 is
  library ieee;
  use ieee.std_logic_1164.all;
end context ctx1;

package generic_pkg is
  generic ( type T );
  function ident (x : T) return T;
end package generic_pkg;

package body generic_pkg is
  function ident (x : T) return T is
  begin
    return x;
  end function ident;
end package body generic_pkg;

package int_pkg is new work.generic_pkg generic map (T => integer);

package q is
  function int_ident is new int_pkg.ident generic map (T => integer);
end package q;

package uses_pkg_generic is
  generic ( package g is new work.generic_pkg generic map (<>) );
  function "+" (a, b : integer) return integer;
  alias add_alias is "+"[integer, integer return integer];
end package uses_pkg_generic;

package coverage_pkg is
  type distance is range 0 to 1e9
    units
      um;
      mm = 1000 um;
      m = 1000 mm;
    end units;

  type rec_t is record
    a : integer;
    b : bit;
  end record;

  type shared_counter is protected
    procedure incr;
    impure function value return integer;
  end protected;

  alias b_alias is bit;

  group reg_pins is (signal, signal);
end package coverage_pkg;

package body coverage_pkg is
  type shared_counter is protected body
    variable v : integer := 0;
    procedure incr is
    begin
      v := v + 1;
    end procedure incr;
    impure function value return integer is
    begin
      return v;
    end function value;
  end protected body;
end package body coverage_pkg;

entity buf is
  port ( d : in bit; q : out bit );
end entity buf;

architecture rtl of buf is
begin
  q <= d;
end architecture rtl;

entity \top-level\ is
  port ( clk : in bit; q : out bit );
end entity \top-level\;

architecture rtl of \top-level\ is
  component buf is
    port ( d : in bit; q : out bit );
  end component buf;

  signal s, g : bit;
  signal s1, s2 : bit;

  disconnect s : bit after 5 ns;

  group my_regs : reg_pins (s1, s2);
begin

  blk1 : block (g = '1')
  begin
    s <= guarded '0';
  end block blk1;

  s <= force '1' when g = '1' else '0';

  proc1 : process (all) is
  begin
    s <= release;
    wait;
  end process proc1;

  proc2 : postponed process is
  begin
  end postponed process proc2;

  inst1 : buf port map (d => clk, q => q);

end architecture rtl;

configuration cfg1 of \top-level\ is
  for rtl
    for inst1 : buf
      use entity work.buf(rtl);
    end for;
  end for;
end configuration cfg1;
