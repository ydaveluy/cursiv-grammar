note
	description: "Exercises INSPECT, ACROSS ... ALL/SOME, object tests, %
			%ATTACHED and quantified expressions."

class
	INSPECTOR

feature -- Access

	describe (code: INTEGER): STRING
		do
			inspect code
			when 1, 2, 3 then
				Result := "small"
			when 4..9 then
				Result := "middling"
			when 10, 20..29 then
				Result := "round"
			else
				Result := "other"
			end
		end

	all_positive (values: ARRAY [INTEGER]): BOOLEAN
		do
			Result := across values as v all v.item > 0 end
		end

	any_negative (values: ARRAY [INTEGER]): BOOLEAN
		do
			Result := across values as v some v.item < 0 end
		end

	describe_object (obj: ANY): STRING
		do
			if attached {STRING} obj as s then
				Result := s
			elseif attached obj as a then
				Result := "something"
			else
				Result := "void"
			end
		end

	all_items_valid (values: ARRAY [INTEGER]): BOOLEAN
		do
			Result := ∀ i: values ¦ i > 0
		end

end
