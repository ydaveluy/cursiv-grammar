#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "muddb/core/ast.hpp"

namespace muddb {

struct Muddb : cursiv::GrammarSpec {
  struct EOL : terminal<lit<"\r\n"> | lit<"\n"> | (lit<"\r"> + !lit<"\n">)> {};

  struct END : terminal<lit<"***END OF DUMP***">> {};

  struct KEY_NUMBER : terminal<some(d)> {};
  struct LEGACY_KEY : terminal<lit<"-"> + many(cr<R"(^\r\n)">)> {};
  struct ROOM_NO : terminal<lit<"#"> + many(cr<R"(^\r\n)">)> {};

  struct STRING : terminal<some(cr<R"(^\r\n)">)> {};
  struct EXTRA_STRING
      : terminal<!cr<"#*"> + some(cr<R"(^\r\n)">)> {};

  struct Key;
  struct ReferenceKey
      : rule<create<ast::KeyReference>(
            assign<&ast::KeyReference::value>(ref<KEY_NUMBER>))> {};
  struct LegacyKey : rule<create<ast::LegacyKey>(
                             assign<&ast::LegacyKey::text>(ref<LEGACY_KEY>))> {
  };
  struct NotKey
      : rule<create<ast::KeyNot>(lit<"("> + lit<"!"> +
                                 assign<&ast::KeyNot::operand>(ref<Key>) +
                                 lit<")">)> {};
  struct BinaryKey
      : rule<create<ast::KeyBinary>(
            lit<"("> + assign<&ast::KeyBinary::left>(ref<Key>) +
            assign<&ast::KeyBinary::op>(lit<"&"> | lit<"|">) +
            assign<&ast::KeyBinary::right>(ref<Key>) + lit<")">)> {};
  struct Key
      : rule<ref<ReferenceKey> | ref<NotKey> | ref<BinaryKey>> {};

  struct ExtraLine : rule<create<ast::ExtraLine>(
                             option(assign<&ast::ExtraLine::text>(ref<EXTRA_STRING>)) +
                             ref<EOL>)> {};

  struct RoomNo
      : rule<assign<&ast::Room::roomno>(ref<ROOM_NO>) + ref<EOL>> {};

  struct Room
      : rule<ref<RoomNo> + option(assign<&ast::Room::name>(ref<STRING>)) +
             ref<EOL> + option(assign<&ast::Room::description>(ref<STRING>)) +
             ref<EOL> + option(assign<&ast::Room::location>(ref<STRING>)) +
             ref<EOL> + option(assign<&ast::Room::contents>(ref<STRING>)) +
             ref<EOL> + option(assign<&ast::Room::exits>(ref<STRING>)) +
             ref<EOL> + option(assign<&ast::Room::next_room>(ref<STRING>)) +
             ref<EOL> +
             option(
                 assign<&ast::Room::key>(ref<LegacyKey> | ref<Key>)) +
             ref<EOL> + option(assign<&ast::Room::fail_message>(ref<STRING>)) +
             ref<EOL> + option(assign<&ast::Room::succ_message>(ref<STRING>)) +
             ref<EOL> + option(assign<&ast::Room::ofail>(ref<STRING>)) +
             ref<EOL> + option(assign<&ast::Room::osuccess>(ref<STRING>)) +
             ref<EOL> + option(assign<&ast::Room::owner>(ref<STRING>)) +
             ref<EOL> + option(assign<&ast::Room::pennies>(ref<STRING>)) +
             ref<EOL> + option(assign<&ast::Room::flags>(ref<STRING>)) +
             ref<EOL> + option(assign<&ast::Room::password>(ref<STRING>)) +
             ref<EOL> + many(append<&ast::Room::extras>(ref<ExtraLine>))> {
  };

  struct Db : rule<many(append<&ast::Db::rooms>(ref<Room>)) + ref<END> +
                       many(ref<EOL>) + eof> {};

  using entry = Db;
};

} // namespace muddb
