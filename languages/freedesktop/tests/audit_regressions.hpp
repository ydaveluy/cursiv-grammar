#pragma once
#include "cursiv/core/parser/Parser.hpp"
#include "desktopentry/core/CoreServices.hpp"
#include <array>
#include <string>
#include <utility>
#include <vector>
namespace audit_regressions {
inline bool run(const desktopentry::DesktopEntryCoreServices& services) {
  struct Group { std::string name; std::vector<std::pair<std::string, std::string>> entries; };
  struct Case { const char* source; std::vector<Group> groups; };
  const std::array cases = {
    Case{"[Desktop Entry]\nName=App\nEmpty=\n", {Group{"Desktop Entry", {{"Name","App"},{"Empty",""}}}}},
    Case{"[Équipe]\nNom=bonjour\n", {Group{"Équipe", {{"Nom","bonjour"}}}}},
    Case{"[Desktop Entry]\n123=number\nÉtiquette=oui\nX_Name=under\n", {Group{"Desktop Entry", {{"123","number"},{"Étiquette","oui"},{"X_Name","under"}}}}},
    Case{"[Desktop Entry]\nName[es_419]=Hola\nName[sr_RS.UTF-8@latin]=Zdravo\n", {Group{"Desktop Entry", {{"Name[es_419]","Hola"},{"Name[sr_RS.UTF-8@latin]","Zdravo"}}}}},
    Case{"[ spaced name ]\nName=  initial and final  \n", {Group{" spaced name ", {{"Name","initial and final  "}}}}},
    Case{"[Desktop Entry]\nMimeType=text/plain;image/png;\nExec=run %F --url=https://a.test?q=x=y\n", {Group{"Desktop Entry", {{"MimeType","text/plain;image/png;"},{"Exec","run %F --url=https://a.test?q=x=y"}}}}},
  };
  for (const auto& test : cases) {
    const std::string source(test.source);
    const auto result = services.parser->parse(source);
    const auto* doc = result.value_as<desktopentry::ast::Document>();
    if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty() ||
        !doc || doc->groups.size() != test.groups.size())
      return false;
    for (std::size_t i = 0; i < test.groups.size(); ++i) {
      const auto* group = doc->groups[i];
      const auto& expected = test.groups[i];
      if (group->name != expected.name || group->parent != doc ||
          group->entries.size() != expected.entries.size())
        return false;
      for (std::size_t j = 0; j < expected.entries.size(); ++j) {
        const auto* entry = group->entries[j];
        std::string key(entry->key);
        if (entry->locale) {
          const auto* locale = entry->locale;
          if (locale->parent != entry)
            return false;
          key += '[';
          key += locale->language;
          if (!locale->country.empty()) { key += '_'; key += locale->country; }
          if (!locale->encoding.empty()) { key += '.'; key += locale->encoding; }
          if (!locale->modifier.empty()) { key += '@'; key += locale->modifier; }
          key += ']';
        }
        if (key != expected.entries[j].first ||
            entry->rawValue != expected.entries[j].second || entry->parent != group)
          return false;
      }
    }
  }
  return true;
}
}
