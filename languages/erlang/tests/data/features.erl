-module(features).

-export([run/0]).

-record(point, {x, y = 0}).
-record(typed_point, {x :: integer(), y = 0 :: integer()}).

-type coord() :: integer() | float().
-type pair(A, B) :: {A, B}.

-spec add(integer(), integer()) -> integer();
         (float(), float()) -> float() when is_subtype(float(), number()).
add(X, Y) -> X + Y.

-callback handle(term()) -> ok | {error, term()}.

quoted_atoms() ->
    'an atom with spaces' = 'an atom with spaces',
    Char = $\n,
    Hex = 16#FF,
    Pi = 3.14159,
    Neg = -42,
    {Char, Hex, Pi, Neg}.

lists_and_binaries() ->
    List = [1, 2, 3 | [4, 5]],
    Squares = [X * X || X <- List, X rem 2 =:= 0],
    Bin = <<1, 2, 3:16, "abc"/binary>>,
    BinVals = [B || <<B:8>> <= Bin],
    {List, Squares, Bin, BinVals}.

records_and_maps() ->
    P = #point{x = 1, y = 2},
    P2 = P#point{y = 3},
    X = P2#point.x,
    M = #{a => 1, b => 2},
    M2 = M#{a := 10, c => 3},
    {P, P2, X, M, M2}.

funs_and_calls() ->
    Local = fun add/2,
    Remote = fun lists:reverse/1,
    Named = fun Fact(0) -> 1; Fact(N) when N > 0 -> N * Fact(N - 1) end,
    Anon = fun(A, B) -> A ++ B end,
    {Local(1, 2), Remote([1, 2, 3]), Named(5), Anon("ab", "cd")}.

control_flow(X) ->
    R1 = if X > 0 -> positive; X < 0 -> negative; true -> zero end,
    R2 = case X of
             0 -> zero;
             N when N > 0, N < 10 -> small;
             _ -> other
         end,
    R3 = try 1 / X of
             V -> {ok, V}
         catch
             error:badarith -> {error, badarith};
             Class:Reason:Stack -> {Class, Reason, Stack}
         after
             ok
         end,
    R4 = catch (1 / X),
    {R1, R2, R3, R4}.

messaging() ->
    self() ! hello,
    receive
        hello -> ok
    after 100 ->
        timeout
    end.

logic() ->
    A = true andalso false,
    B = true orelse false,
    C = 1 =< 2 andalso 2 >= 1,
    D = [1, 2] ++ [3] -- [2],
    {A, B, C, D}.

run() ->
    quoted_atoms(),
    lists_and_binaries(),
    records_and_maps(),
    funs_and_calls(),
    control_flow(1),
    logic(),
    ok.
