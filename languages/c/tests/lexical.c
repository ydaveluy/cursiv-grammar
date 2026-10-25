int \u03b1 = 1;
int β = 2;
int intβ = 3;
unsigned char utf8 = u8'a';
const char *spliced = "one\
two";

[[]] int empty_attribute;
[[,,,,]] int empty_attributes;
[[, maybe_unused,,]] int sparse_attributes;
enum [[]] Annotated { VALUE [[]] };

// Continued comment \
this text is still inside the comment
int after_comment;
