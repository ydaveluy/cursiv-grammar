from pathlib import Path
import argparse
import ctypes
import ctypes.util
import re

parser = argparse.ArgumentParser()
parser.add_argument("--output", type=Path, default=Path(__file__).resolve().parents[1] /
                    "src/graphemes/core/GraphemeProperties.hpp")
output = parser.parse_args().output
library = ctypes.util.find_library("icuuc")
if not library:
    raise SystemExit("ICU is required to regenerate the property table.")
icu = ctypes.CDLL(library)
major = int(re.search(r"\d+", library).group())


def function(name, arguments, result):
    fn = getattr(icu, f"{name}_{major}")
    fn.argtypes, fn.restype = arguments, result
    return fn


property_id = function("u_getPropertyEnum", [ctypes.c_char_p], ctypes.c_int)
property_value = function("u_getPropertyValueEnum", [ctypes.c_int, ctypes.c_char_p], ctypes.c_int)
get_value = function("u_getIntPropertyValue", [ctypes.c_int32, ctypes.c_int], ctypes.c_int)
gcb = property_id(b"Grapheme_Cluster_Break")
incb = property_id(b"Indic_Conjunct_Break")
pictographic = property_id(b"Extended_Pictographic")
if min(gcb, incb, pictographic) < 0:
    raise SystemExit("This ICU version lacks the required Unicode properties.")
names = ["Control", "Extend", "ZWJ", "Regional_Indicator", "Prepend", "SpacingMark", "L", "V", "T", "LV", "LVT"]
gcb_names = {property_value(gcb, name.encode()): name for name in names}
incb_names = {property_value(incb, name.encode()): "InCB" + name for name in ["Consonant", "Extend", "Linker"]}
ranges = {name: [] for name in names + list(incb_names.values()) + ["Extended_Pictographic"]}


def append(name, point):
    spans = ranges[name]
    if spans and spans[-1][1] + 1 == point:
        spans[-1][1] = point
    else:
        spans.append([point, point])


for point in range(0x110000):
    if 0xD800 <= point <= 0xDFFF:
        continue
    breaking, conjunct = get_value(point, gcb), get_value(point, incb)
    if breaking in gcb_names:
        append(gcb_names[breaking], point)
    if conjunct in incb_names:
        append(incb_names[conjunct], point)
    if get_value(point, pictographic):
        append("Extended_Pictographic", point)


def version(name):
    values = (ctypes.c_uint8 * 4)()
    function(name, [ctypes.POINTER(ctypes.c_uint8)], None)(values)
    parts = list(values)
    while parts and parts[-1] == 0:
        parts.pop()
    return ".".join(map(str, parts))


def spelling(point):
    return "\\\\u%04X" % point if point <= 0xFFFF else "\\U%08X" % point


lines = ["#pragma once", "", '#include "cursiv/core/grammar/expr.hpp"', "",
         "namespace graphemes::properties {",
         f"// Generated from ICU {version('u_getVersion')} / Unicode {version('u_getUnicodeVersion')} property values."]
for name, spans in ranges.items():
    spec = "".join(spelling(first) + ("-" + spelling(last) if first != last else "") for first, last in spans)
    lines.append(f'inline constexpr auto {name} = cursiv::uc::cr<"{spec}">;')
lines.append("}  // namespace graphemes::properties\n")
output.write_text("\n".join(lines))
print(output)
