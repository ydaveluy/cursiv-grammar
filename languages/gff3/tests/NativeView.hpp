#pragma once
#include "cursiv/core/services/JsonValue.hpp"
#include "gff3/core/Value.hpp"
#include "gff3/core/ast.hpp"
#include <string>
using J = cursiv::JsonValue;
inline std::string unquote(std::string_view raw) {
  const auto decoded = gff3::attribute_value(raw);
  return decoded ? *decoded : std::string(raw);
}
inline J coordinate(std::string_view raw) {
  if (raw.empty() || raw == ".")
    return nullptr;
  bool negative = raw.front() == '-';
  if (negative || raw.front() == '+')
    raw.remove_prefix(1);
  auto i = raw.find_first_not_of('0');
  if (i == raw.npos)
    return "0";
  return (negative ? "-" : "") + std::string(raw.substr(i));
}
inline J feature_view(const gff3::ast::Feature &f) {
  J::Object attributes;
  if (f.attributesMissing)
    attributes.emplace(".", J::Array{});
  for (auto *a : f.attributes) {
    J::Array values;
    if (!(a->values.size() == 1 && a->values[0].empty()))
      for (auto raw : a->values)
        values.emplace_back(unquote(raw));
    auto key = std::string(a->key);
    if (auto it = attributes.find(key); it != attributes.end()) {
      auto arr = it->second.array();
      arr.insert(arr.end(), values.begin(), values.end());
      it->second = J(std::move(arr));
    } else
      attributes.emplace(key, J(std::move(values)));
  }
  J::Array extra;
  for (auto x : f.extra)
    extra.emplace_back(x);
  return J::Object{{"seqid", f.seqid},
                   {"source", f.source},
                   {"type", f.type},
                   {"start", coordinate(f.start)},
                   {"end", coordinate(f.end)},
                   {"score", f.score},
                   {"strand", f.strand},
                   {"phase", f.phase},
                   {"attributes", std::move(attributes)},
                   {"extra", std::move(extra)}};
}
inline J features_view(const gff3::ast::Document &doc) {
  J::Array fs;
  for (const auto &line : doc.lines)
    if (const auto *f = std::get_if<gff3::ast::Feature *>(&line); f && *f)
      fs.push_back(feature_view(**f));
  return fs;
}

inline J fasta_view(const gff3::ast::Document &doc) {
  J::Array sequences;
  if (doc.fasta)
    for (auto *s : doc.fasta->sequences) {
      std::string bases;
      for (auto line : s->lines)
        bases += line;
      sequences.emplace_back(
          J::Object{{"name", s->name}, {"sequence", std::move(bases)}});
    }
  return sequences;
}

inline J directives_view(const gff3::ast::Document &doc) {
  J::Array ds;
  for (const auto &line : doc.lines)
    if (auto *c = std::get_if<gff3::ast::CommentLine *>(&line);
        c && *c && (*c)->text.starts_with("##"))
      ds.emplace_back((*c)->text.substr(2));
  return ds;
}

inline J document_view(const gff3::ast::Document &doc) {
  return J::Object{{"features", features_view(doc)},
                   {"directives", directives_view(doc)},
                   {"fasta", fasta_view(doc)}};
}
