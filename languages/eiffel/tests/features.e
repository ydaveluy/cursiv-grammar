note
	description: "Exercises generics, TUPLE, agents, once/external routines, %
			%converters, and inheritance adaptation."

deferred class
	CONTAINER [G -> COMPARABLE create make end]

inherit
	ANY
		redefine
			default_create
		rename
			out as to_string
		export
			{NONE} all
		end

create
	make, make_from_tuple

feature {NONE} -- Initialization

	make
		do
			create items.make (0)
		end

	make_from_tuple (t: TUPLE [x: INTEGER; y: INTEGER])
		local
			pair: TUPLE [a: INTEGER; b: INTEGER]
		do
			pair := t
			create items.make (pair.a)
		end

feature -- Access

	items: ARRAY [G]

	count: INTEGER
		once
			Result := items.count
		end

	print_all
		do
			items.do_all (agent print_one)
		ensure
			printed: True
		end

	print_one (g: G)
		external
			"C"
		alias
			"eiffel_print"
		end

	first_matching (target: G): detachable G
		local
			i: INTEGER
		do
			from
				i := 1
			invariant
				i >= 1
			variant
				items.count - i
			until
				i > items.count
			loop
				if items.item (i) ~ target then
					Result := items.item (i)
				end
				i := i + 1
			end
		end

invariant
	count_non_negative: count >= 0

end
