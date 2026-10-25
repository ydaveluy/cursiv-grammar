#include "cursiv/core/services/JsonValue.hpp"
#include "rcs/core/ast.hpp"
using J = cursiv::JsonValue;
inline J hx(std::string_view s) {
  std::string out;
  for (unsigned char c : s) {
    out += "0123456789abcdef"[c >> 4];
    out += "0123456789abcdef"[c & 15];
  }
  return out;
}
inline J phrases_view(const auto &ps) {
  J::Array out;
  for (auto *p : ps) {
    J::Array words;
    for (auto w : p->words)
      words.emplace_back(hx(w));
    out.emplace_back(words);
  }
  return out;
}
inline J file_view(const rcs::ast::File &f) {
  if (!f.admin)
    return nullptr;
  const auto &a = *f.admin;
  J::Array access, symbols, locks, deltas;
  for (auto x : a.access)
    access.emplace_back(hx(x));
  for (auto *s : a.symbols)
    symbols.emplace_back(J::Array{hx(s->name), hx(s->revision)});
  for (auto *s : a.locks)
    locks.emplace_back(J::Array{hx(s->name), hx(s->revision)});
  for (auto *d : f.deltas) {
    J::Array branches;
    for (auto s : d->branches)
      branches.emplace_back(hx(s));
    J commit = d->commitid ? hx(*d->commitid) : J(nullptr), log = nullptr,
      text = nullptr;
    for (auto *t : f.deltatexts)
      if (t->revision == d->revision) {
        log = hx(t->log);
        text = hx(t->text);
      }
    deltas.emplace_back(J::Object{{"revision", hx(d->revision)},
                                  {"date", hx(d->date)},
                                  {"author", hx(d->author)},
                                  {"state", hx(d->state)},
                                  {"next", d->next ? hx(*d->next) : J(nullptr)},
                                  {"branches", branches},
                                  {"commitid", commit},
                                  {"log", log},
                                  {"text", text}});
  }
  return J::Object{{"head", a.head.empty() ? J(nullptr) : hx(a.head)},
                   {"branch", a.branch ? hx(*a.branch) : J(nullptr)},
                   {"strict", a.strict},
                   {"integrity", a.integrity ? hx(*a.integrity) : J(nullptr)},
                   {"comment", a.comment ? hx(*a.comment) : J(nullptr)},
                   {"expand", a.expand ? hx(*a.expand) : J(nullptr)},
                   {"access", access},
                   {"symbols", symbols},
                   {"locks", locks},
                   {"desc", hx(f.desc)},
                   {"deltas", deltas}};
}
