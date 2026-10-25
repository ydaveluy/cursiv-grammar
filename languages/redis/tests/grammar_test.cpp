#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/test/Check.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include "cursiv/test/ValidationResult.hpp"
#include "redis/core/Argument.hpp"
#include "redis/core/CoreServices.hpp"
#include <cstdio>
#include <filesystem>
#include <string>
#include <vector>
int main() {
  const auto language = cursiv::create_language<redis::RedisCoreServices,
                                                redis::RedisSharedCoreServices,
                                                redis::install_core_module>();
  const auto clean = [&](std::string_view text) {
    const auto r = language.language.parser->parse(text);
    return r.accepted && !r.cst().recovered() && r.diagnostics.empty();
  };
  const char *const valid[] = {
      "GET test",
      "SET test val",
      "SET test val NX EX 60",
      "SET test val XX GET KEEPTTL",
      "DEL a b c",
      "EXISTS a b c",
      "EXPIRE key 10",
      "EXPIRE key 10 NX",
      "EXPIRE key -1 GT",
      "HSET key f1 v1 f2 v2",
      "HRANDFIELD key 3 WITHVALUES",
      "ZADD key GT CH 1 a 2 b",
      "ZADD key NX INCR 1 a",
      "ZRANGEBYSCORE key (5 +inf",
      "ZRANGEBYSCORE key -inf +inf WITHSCORES",
      "ZCOUNT key (1 10",
      "LMOVE a b LEFT RIGHT",
      "LPOS key el RANK 1 COUNT 2 MAXLEN 100",
      "COPY a b DB 1 REPLACE",
      "SCAN 0 MATCH * COUNT 10 TYPE string",
      "SINTERCARD 2 a b LIMIT 5",
      "ZUNIONSTORE dst 2 a b WEIGHTS 1 2 AGGREGATE MAX",
      "RANDOMKEY",
      "WAIT 0 100",
      "GET a\nGET b\n",
      "GET a\nGET b",
      "GET a\n\n\nGET b\n\n",
      "",
      "SET \"my key\" 'my val'",
      "SET \"a\\\"b\" c",
      "get test",
      "Get test",
      "gEt test",
      "set test val nx ex 60"};
  for (auto *s : valid)
    CHECK(clean(s));
  const char *const runtimeInvalid[] = {"GET",
                                        "SET a",
                                        "NOTACOMMAND a b",
                                        "GET a b",
                                        "EXPIRE key",
                                        "SET \"unterminated",
                                        "GET a\nNOTACOMMAND b",
                                        "GET a GET b"};
  for (auto *s : runtimeInvalid) {
    CHECK(clean(s));
    const auto v = cursiv::test::validationHelper(language.language, s);
    CHECK(!v.diagnostics.empty());
  }
  for (auto *s : {"get test", "Get test", "gEt test", "set test val nx ex 60",
                  "ZADD z 1.5 a"})
    CHECK(clean(s));
  const auto check = [&](std::string_view source,
                         std::initializer_list<std::string_view> expected) {
    const auto r = language.language.parser->parse(source);
    CHECK(r.accepted && !r.cst().recovered());
    auto *d = cursiv::ast_cast<redis::ast::Document>(r.value);
    CHECK(d && d->commands.size() == 1);
    if (!d || d->commands.size() != 1)
      return;
    auto *c = d->commands[0];
    CHECK(c->arguments.size() + 1 == expected.size());
    std::size_t i = 0;
    for (auto want : expected) {
      const auto *v = i ? c->arguments[i - 1] : c->name;
      CHECK(v && redis::argument_value(*v).text == want);
      ++i;
    }
  };
  check("SET NX XX", {"SET", "NX", "XX"});
  check("SET mykey myval NX GET EX 60",
        {"SET", "mykey", "myval", "NX", "GET", "EX", "60"});
  check("EXPIRE key -1 GT", {"EXPIRE", "key", "-1", "GT"});
  check("HEXPIRE h 100 NX FIELDS 2 f1 f2",
        {"HEXPIRE", "h", "100", "NX", "FIELDS", "2", "f1", "f2"});
  check("ZCOUNT key (5 +inf", {"ZCOUNT", "key", "(5", "+inf"});
  check("SET \"my key\" \"a\\\"b\"", {"SET", "my key", "a\"b"});
  check("SCAN 0 MATCH * COUNT 1 TYPE string",
        {"SCAN", "0", "MATCH", "*", "COUNT", "1", "TYPE", "string"});
  const std::string multi = "SET a 1\nGET a\nDEL a\n";
  const auto result = language.language.parser->parse(multi);
  auto *d = cursiv::ast_cast<redis::ast::Document>(result.value);
  CHECK(d && d->commands.size() == 3);
  if (d && d->commands.size() == 3) {
    CHECK(redis::argument_value(*d->commands[0]->name).text == "SET");
    CHECK(redis::argument_value(*d->commands[1]->name).text == "GET");
    CHECK(redis::argument_value(*d->commands[2]->name).text == "DEL");
  }
  unsigned files = 0;
  for (const auto &e : std::filesystem::directory_iterator(
           std::string(CURSIV_EXAMPLE_DIR) + "/example"))
    if (e.path().extension() == ".redis") {
      const auto source = cursiv::test::read_file(e.path().string());
      CHECK(clean(source));
      ++files;
    }
  CHECK(files == 60);
  return cursiv::test::report("redis_grammar_test");
}
