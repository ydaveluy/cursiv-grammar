#pragma once
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include "toml/core/CoreServices.hpp"
#include "toml/core/ast.hpp"
#include <cmath>
#include <cstdio>
#include <limits>
#include <string>
#include <string_view>
#include <variant>
namespace value_regressions {
inline bool run() {
  namespace ast = toml::ast;
  const auto language = cursiv::create_language<toml::TomlCoreServices,
                                                toml::TomlSharedCoreServices,
                                                toml::install_core_module>();
  bool ok = true;
  const auto check = [&](bool pass, const char *message) {
    if (!pass) {
      std::fprintf(stderr, "toml values: %s\n", message);
      ok = false;
    }
  };
  const auto clean = [](const cursiv::ParseResult &r) {
    return r.accepted && !r.cst().recovered() && r.diagnostics.empty();
  };
  const auto entry = [](const cursiv::ParseResult &r) -> const ast::KeyValue * {
    const auto *d = r.value_as<ast::Document>();
    return d && d->tables.size() == 1 && d->tables[0]->entries.size() == 1
               ? d->tables[0]->entries[0]
               : nullptr;
  };
  struct StringCase {
    const char *source;
    std::string_view value;
  };
  for (const auto &c : {
           StringCase{"v = \"a\\nb\\t\\\"\\\\\\b\\f\\r\"\n",
                      std::string_view{"a\nb\t\"\\\b\f\r", 9}}, // basic_escapes
           StringCase{"v = \"\\u0061\\U0001F600\"\n",
                      std::string_view{"a😀", 5}},            // basic_unicode
           StringCase{"v = \"\"\n", std::string_view{"", 0}}, // basic_empty
           StringCase{"v = 'a\\nb'\n", std::string_view{"a\\nb", 4}}, // literal
           StringCase{"v = \"\"\"\nalpha\nbeta\"\"\"\n",
                      std::string_view{"alpha\nbeta", 10}}, // ml_basic_newline
           StringCase{"v = \"\"\"\r\nalpha\r\nbeta\"\"\"\r\n",
                      std::string_view{"alpha\nbeta", 10}}, // ml_basic_crlf
           StringCase{"v = \"\"\"a\"\"\"\"\n",
                      std::string_view{"a\"", 2}}, // ml_basic_four_quotes
           StringCase{"v = \"\"\"a\"\"\"\"\"\n",
                      std::string_view{"a\"\"", 3}}, // ml_basic_five_quotes
           StringCase{"v = \"\"\"one \"two\" and \"\"three\"\" end\"\"\"\n",
                      std::string_view{"one \"two\" and \"\"three\"\" end",
                                       27}}, // ml_basic_internal_quotes
           StringCase{"v = \"\"\"a\\\n  \n  b\"\"\"\n",
                      std::string_view{"ab", 2}}, // ml_basic_fold
           StringCase{"v = \"\"\"a\\ \t\n  \n  b\"\"\"\n",
                      std::string_view{"ab", 2}}, // ml_basic_fold_space
           StringCase{"v = \"\"\"a\\ \t\r\n  \r\n  b\"\"\"\r\n",
                      std::string_view{"ab", 2}}, // ml_basic_fold_crlf
           StringCase{
               "v = '''\nalpha\nbeta'''\n",
               std::string_view{"alpha\nbeta", 10}}, // ml_literal_newline
           StringCase{"v = '''\r\nalpha\r\nbeta'''\r\n",
                      std::string_view{"alpha\nbeta", 10}}, // ml_literal_crlf
           StringCase{"v = '''a''''\n",
                      std::string_view{"a'", 2}}, // ml_literal_four_quotes
           StringCase{"v = '''a'''''\n",
                      std::string_view{"a''", 3}}, // ml_literal_five_quotes
           StringCase{"v = '''one 'two' and ''three'' end'''\n",
                      std::string_view{"one 'two' and ''three'' end",
                                       27}}, // ml_literal_internal_quotes
           StringCase{"v = \"\"\"\"\"\"\n",
                      std::string_view{"", 0}}, // empty_basic_ml
           StringCase{"v = ''''''\n",
                      std::string_view{"", 0}}, // empty_literal_ml
           StringCase{"v=\"\\u0000x\"\n",
                      std::string_view{"\000x", 2}}, // embedded NUL
       }) {
    const auto r = language.language.parser->parse(c.source);
    const auto *kv = entry(r);
    const auto *v = kv ? std::get_if<std::string_view>(&kv->value) : nullptr;
    check(clean(r) && v && *v == c.value,
          "decoded string value, including multiline boundaries and escapes");
    const auto validated =
        cursiv::test::validationHelper(language.language, c.source);
    check(validated.diagnostics.empty(),
          "valid escaped string has no runtime error");
  }
  struct IntegerCase {
    const char *source;
    std::int64_t value;
  };
  for (const IntegerCase c :
       {IntegerCase{"v=-9223372036854775808\n",
                    std::numeric_limits<std::int64_t>::min()},
        IntegerCase{"v=+9223372036854775807\n",
                    std::numeric_limits<std::int64_t>::max()},
        IntegerCase{"v=0x7fff_ffff_ffff_ffff\n",
                    std::numeric_limits<std::int64_t>::max()},
        IntegerCase{"v=0o755\n", 0755}, IntegerCase{"v=0b1010_0011\n", 163}}) {
    const auto r = language.language.parser->parse(c.source);
    const auto *kv = entry(r);
    const auto *v = kv ? std::get_if<std::int64_t>(&kv->value) : nullptr;
    check(clean(r) && v && *v == c.value,
          "integer classification and exact signed/radix value");
  }
  struct FloatCase {
    const char *source;
    double value;
  };
  for (const FloatCase c :
       {FloatCase{"v=+1_000.5e-2\n", 10.005}, FloatCase{"v=-0.0\n", -0.0},
        FloatCase{"v=1e999\n", std::numeric_limits<double>::infinity()},
        FloatCase{"v=-1e999\n", -std::numeric_limits<double>::infinity()},
        FloatCase{"v=1e-999\n", 0.0}, FloatCase{"v=-1e-999\n", -0.0}}) {
    const auto r = language.language.parser->parse(c.source);
    const auto *kv = entry(r);
    const auto *v = kv ? std::get_if<double>(&kv->value) : nullptr;
    check(clean(r) && v && *v == c.value &&
              std::signbit(*v) == std::signbit(c.value),
          "float overflow/underflow and signed zero");
  }
  {
    const auto r = language.language.parser->parse("v=-nan\n");
    const auto *kv = entry(r);
    const auto *v = kv ? std::get_if<double>(&kv->value) : nullptr;
    check(clean(r) && v && std::isnan(*v) && std::signbit(*v),
          "negative NaN retains its sign");
  }
  struct DateCase {
    const char *token;
    ast::DateTimeKind kind;
  };
  for (const DateCase c :
       {DateCase{"1979-05-27T07:32:00Z", ast::DateTimeKind::OffsetDateTime},
        DateCase{"1979-05-27t07:32:00z", ast::DateTimeKind::OffsetDateTime},
        DateCase{"1979-05-27 07:32:00.123456789-07:00",
                 ast::DateTimeKind::OffsetDateTime},
        DateCase{"2024-02-29T07:32:00", ast::DateTimeKind::LocalDateTime},
        DateCase{"2024-02-29", ast::DateTimeKind::LocalDate},
        DateCase{"07:32:00.123456789", ast::DateTimeKind::LocalTime}}) {
    const std::string source = std::string("v=") + c.token + "\n";
    const auto r = language.language.parser->parse(source);
    const auto *kv = entry(r);
    const auto *v = kv ? std::get_if<ast::DateTime>(&kv->value) : nullptr;
    check(clean(r) && v && v->kind == c.kind && v->text == c.token,
          "date/time classification and exact written spelling");
    check(cursiv::test::validationHelper(language.language, source)
              .diagnostics.empty(),
          "valid calendar value");
  }
  {
    const auto r =
        language.language.parser->parse(R"TOML("\u0061" . "b\nc" = "value"
)TOML");
    const auto *kv = entry(r);
    check(clean(r) && kv && kv->key && kv->key->parts.size() == 2 &&
              kv->key->parts[0] == "a" && kv->key->parts[1] == "b\nc",
          "quoted key components decode without joining their paths");
  }
  {
    const auto r = language.language.parser->parse(
        R"TOML(v=["a\nb",[true,2.0],{x=2024-02-29}]
)TOML");
    const auto *kv = entry(r);
    const auto *pv = kv ? std::get_if<ast::Array *>(&kv->value) : nullptr;
    const auto *a = pv ? *pv : nullptr;
    check(clean(r) && a && a->items.size() == 3 &&
              std::get<std::string_view>(a->items[0]) == "a\nb",
          "nested array string value");
    const auto *nested = a && a->items.size() == 3
                             ? std::get<ast::Array *>(a->items[1])
                             : nullptr;
    const auto *inlined = a && a->items.size() == 3
                              ? std::get<ast::InlineTable *>(a->items[2])
                              : nullptr;
    check(nested && nested->parent == a && nested->items.size() == 2 &&
              std::get<bool>(nested->items[0]) && inlined &&
              inlined->parent == a && inlined->entries.size() == 1 &&
              std::holds_alternative<ast::DateTime>(inlined->entries[0]->value),
          "nested structures retain types and parents");
  }
  for (const char *source :
       {"a.b=1\na.c=2\n", "[a.b]\nx=1\n[a]\ny=2\n",
        "[[a]]\nx=1\n[a.b]\ny=1\n[[a]]\nx=2\n[a.b]\ny=2\n", "v={a.b=1,a.c=2}\n",
        "\"a.b\"=1\na.b=2\n", "\"\"=1\n", "true=1\n123=2\n1979-05-27=3\n"}) {
    const auto r = language.language.parser->parse(source);
    check(clean(r) && cursiv::test::validationHelper(language.language, source)
                          .diagnostics.empty(),
          "valid dotted/quoted/array-table namespaces");
  }
  for (const char *source : {"a=1\na=2\n",
                             "a=1\n\"\\u0061\"=2\n",
                             "a=1\na.b=2\n",
                             "v={a=1,a=2}\n",
                             "v={a=1}\nv.b=2\n",
                             "[a]\nx=1\n[a]\ny=2\n",
                             "[a]\nx=1\n[[a]]\ny=2\n",
                             "a.b=1\n[a]\nc=2\n",
                             "v=2023-02-29\n",
                             "v=2024-13-01\n",
                             "v=25:00:00\n",
                             "v=2024-02-29T23:59:59+24:00\n",
                             "v=9223372036854775808\n",
                             "v=0xffff_ffff_ffff_ffff\n",
                             "v=\"\\uD800\"\n",
                             "v=\"\\U00110000\"\n",
                             "v=\"\\q\"\n",
                             "v=\"\\/\"\n",
                             "v=\"\\uZZZZ\"\n",
                             "v=\"a\001b\"\n"}) {
    const auto r = language.language.parser->parse(source);
    check(
        clean(r),
        "invalid scalar/namespace remains parsable without conversion errors");
    const auto validated =
        cursiv::test::validationHelper(language.language, source);
    check(!validated.diagnostics.empty(),
          "invalid scalar/namespace is diagnosed at runtime");
    for (const auto &d : validated.diagnostics)
      check(d.range.end > d.range.begin,
            "runtime diagnostics have source spans");
  }
  {
    const std::string source = R"TOML(v=["\q",2023-02-29,9223372036854775808]
)TOML";
    const auto validated =
        cursiv::test::validationHelper(language.language, source);
    check(validated.diagnostics.size() == 3,
          "all array scalar positions are validated");
  }
  return ok;
}
} // namespace value_regressions
