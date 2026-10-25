### FILE="scratch.agc"
# Exercises constructs the grammars-v4 corpus does not reach: the
# two-opcodes-on-a-line format, the AXT pseudo-ops, a comment row between
# an opcode row and its first operand row, and a parenthesised index on a
# name that is itself an opcode spelling.

		BANK	7

TWOOP		LODON	TSLT
			# an operand row follows this comment row
			5

AXTEX		AXT,1	SOMELBL
			AXT,2	OTHRLBL

CA(1)		EQUALS	7

		TC	ENDOFJOB
