#pragma once

#include <cstdio>
#include <functional>
#include <string>
#include <unordered_set>

#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "cursiv/core/workspace/Document.hpp"
#include "wavefront/core/CoreServices.hpp"

namespace code_audit {
namespace ast = wavefront::ast;
inline bool run() {
  const auto language =
      cursiv::create_language<wavefront::WavefrontCoreServices,
                              wavefront::WavefrontSharedCoreServices,
                              wavefront::install_core_module>();
  bool ok = true;
  auto check = [&](bool value, const char* message) {
    if (!value) {
      std::fprintf(stderr, "wavefront code audit: %s\n", message);
      ok = false;
    }
  };
  auto parse = [&](std::string_view source) {
    auto r = language.language.parser->parse(source);
    check(r.accepted && !r.cst().recovered() && r.diagnostics.empty(),
          "clean parse");
    check(r.value_as<ast::Document>() != nullptr, "Document root");
    std::unordered_set<const cursiv::AstNode*> reached;
    std::function<void(const cursiv::AstNode*, const cursiv::AstNode*)> visit =
        [&](const auto* n, const auto* parent) {
          if (!n) return;
          check(n->parent == parent, "AST parents");
          if (!reached.insert(n).second) {
            check(false, "unique ownership");
            return;
          }
          language.language.astReflection->visitMembers(
              *n, [&](auto, auto, auto& child) { visit(&child, n); });
        };
    visit(r.value, nullptr);
    check(reached.size() == r.nodes.size(), "all allocated nodes reachable");
    return r;
  };
  for (std::string_view text : {"", "# comment", "\xef\xbb\xbf# comment\r\n"}) {
    const auto r = parse(text);
    const auto* d = r.value_as<ast::Document>();
    check(d && d->statements.empty(), "empty documents and BOM");
  }
  {
    constexpr std::string_view source =
        "v 0 0 0 .1 .2 .3\nv 1 0 0 .4 .5 .6 .7\nv 0 1 0 2\n"
        "vt 0 0 .1\nvt 1 0 .2\nvt 0 1 .3\nvn 0 0 1\n"
        "f -3/-3/-1 -2//-1 -1/-1/\nl 1/1/1 2//1\np 1/1/1 3\n"
        "g\ng a.b c:d α/x #hash\no mesh: α/b.obj  with  spaces\n"
        "usemtl ruby red #part\nmtllib a.mtl b.mtl\n"
        "vendor arbitrary words 1/2\n";
    const auto r = parse(source);
    const auto* d = r.value_as<ast::Document>();
    check(d && d->statements.size() == 16, "every statement retained");
    if (d && d->statements.size() == 16) {
      const auto* rgb = cursiv::ast_cast<ast::Vertex>(d->statements[0]);
      const auto* rgba = cursiv::ast_cast<ast::Vertex>(d->statements[1]);
      const auto* weighted = cursiv::ast_cast<ast::Vertex>(d->statements[2]);
      check(rgb && rgb->color.size() == 3 && rgb->color[1] == .2 && !rgb->w &&
                rgba && rgba->color.size() == 4 && rgba->color[3] == .7 &&
                !rgba->w && weighted && weighted->w == 2 &&
                weighted->color.empty(),
            "RGB, RGBA and weight have distinct fields");
      const auto* f = cursiv::ast_cast<ast::Faces>(d->statements[7]);
      check(f && f->v.size() == 3 && f->v[0]->vertex == "-3" &&
                f->v[0]->texture == "-3" && f->v[0]->normal == "-1" &&
                f->v[1]->texture && f->v[1]->texture->empty() &&
                f->v[1]->normal == "-1" && f->v[2]->texture == "-1" &&
                f->v[2]->normal && f->v[2]->normal->empty(),
            "signed indices and empty slash slots retained");
      const auto* g0 = cursiv::ast_cast<ast::Group>(d->statements[10]);
      const auto* g = cursiv::ast_cast<ast::Group>(d->statements[11]);
      const auto* o = cursiv::ast_cast<ast::ObjectName>(d->statements[12]);
      const auto* m = cursiv::ast_cast<ast::UseMaterial>(d->statements[13]);
      const auto* other =
          cursiv::ast_cast<ast::OtherStatement>(d->statements[15]);
      check(g0 && g0->names.empty() && g && g->names.size() == 4 &&
                g->names[0] == "a.b" && g->names[3] == "#hash" && o &&
                o->name == "mesh: α/b.obj  with  spaces" && m &&
                m->name == "ruby red #part" && other &&
                other->keyword == "vendor" &&
                other->arguments == "arbitrary words 1/2",
            "raw names, reset groups and extensions retained");
    }
  }
  {
    const auto r = parse("mtllib a\\ b.mtl c.mtl\n");
    const auto* d = r.value_as<ast::Document>();
    const auto* m =
        d && d->statements.size() == 1
            ? cursiv::ast_cast<ast::MaterialLibrary>(d->statements[0])
            : nullptr;
    check(m && m->filenames.size() == 2 && m->filenames[0] == "a b.mtl" &&
              m->filenames[1] == "c.mtl",
          "escaped spaces belong to the library filename");
  }
  {
    const auto r = parse("v 0 0 0\nv 1 0 0\nv 0 1 0\nf 1/ 2// 3\n");
    const auto* d = r.value_as<ast::Document>();
    const auto* f = d && d->statements.size() == 4
                        ? cursiv::ast_cast<ast::Faces>(d->statements[3])
                        : nullptr;
    check(f && f->v.size() == 3 && f->v[0]->texture &&
              f->v[0]->texture->empty() && !f->v[0]->normal &&
              f->v[1]->normal && f->v[1]->normal->empty() &&
              !f->v[2]->texture && !f->v[2]->normal,
          "missing slots cannot consume the next corner");
  }
  {
    const auto r = parse(
        "vp 0 0\nvp 1 0\ncstype rat bezier\ndeg 1\n"
        "curv2 -2 -1\nparm u 0 1\nend\n"
        "v 0 0 0\nv 1 0 0\nv 0 1 0\ndeg 1 1\n"
        "surf 0 1 0 1 1 2 3\nparm u 0 1\nparm v 0 1\n"
        "trim 0 1 1\nhole .1 .8 2\nscrv .2 .3 3\nend\n");
    const auto* d = r.value_as<ast::Document>();
    const auto* c =
        d && d->statements.size() == 10
            ? cursiv::ast_cast<ast::FreeFormSurface>(d->statements[4])
            : nullptr;
    const auto* s =
        d && d->statements.size() == 10
            ? cursiv::ast_cast<ast::FreeFormSurface>(d->statements[9])
            : nullptr;
    const auto* h = c ? cursiv::ast_cast<ast::Curve2D>(c->head) : nullptr;
    check(h && h->vp.size() == 2 && h->vp[0] == -2 && c->body.size() == 1 &&
              s && cursiv::ast_cast<ast::Surface>(s->head) &&
              s->body.size() == 5 &&
              cursiv::ast_cast<ast::OuterTrimmingLoop>(s->body[2]) &&
              cursiv::ast_cast<ast::InnerTrimmingLoop>(s->body[3]) &&
              cursiv::ast_cast<ast::SpecialCurve>(s->body[4]),
          "curve and surface bodies retain their roles");
  }
  {
    const std::string source =
        "f +1 +2 +3\nv 0 0 0\nv 1 0 0\nv 0 1 0\nf -3 -2 -1\n";
    cursiv::Document doc(
        cursiv::TextDocument::create("memory:obj", "wavefront", 0, source));
    doc.parseResult = parse(source);
    unsigned errors = 0;
    auto sink = [&](cursiv::Diagnostic&&) { ++errors; };
    cursiv::ValidationAcceptor accept(sink, doc, source);
    if (const auto* d = doc.parseResult.value_as<ast::Document>())
      language.language.validator->checkDocument(*d, accept);
    check(errors == 0,
          "forward absolute and current relative indices validate");
  }
  {
    const std::string source =
        "v nan +inf -Infinity\nv 0 0 0 .1 .2\np\nl 0\n"
        "f 9223372036854775808 -9 1\n";
    cursiv::Document doc(
        cursiv::TextDocument::create("memory:obj", "wavefront", 0, source));
    doc.parseResult = parse(source);
    unsigned errors = 0;
    auto sink = [&](cursiv::Diagnostic&&) { ++errors; };
    cursiv::ValidationAcceptor accept(sink, doc, source);
    const auto* d = doc.parseResult.value_as<ast::Document>();
    if (d) language.language.validator->checkDocument(*d, accept);
    for (const auto* n : doc.parseResult.nodes)
      if (const auto* p = cursiv::ast_cast<ast::Vertex>(n))
        language.language.validator->checkVertex(*p, accept);
    check(errors == 9, "invalid values and arities receive runtime errors");
  }
  return ok;
}
}  // namespace code_audit
