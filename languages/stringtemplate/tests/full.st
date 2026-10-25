<!a template comment, wrapped in the delimiters!>
// a line comment outside any delimiter
/* a block comment outside any delimiter */
Hello <name>!
<if(items)>
<items:{it|<it.value>}; separator=", ">
<elseif(!ready || busy)>
not ready
<else>
default
<endif>
<@header>
Header
<@end>
<\n><\t><\\>
<obj.field.(dyn)>
<f(x, name="x", ...)>
<super.tmpl(a, b)>
<@super.region()>
<(name)(arg1, arg2)>
<[x, y, "three", true, false]>
<a, b : t1 : t2, t3>
