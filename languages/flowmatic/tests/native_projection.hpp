#pragma once

#include "flowmatic/core/ast.hpp"
#include <ostream>
#include <string>

namespace native_regressions {
using namespace flowmatic;
class Projection {
  std::ostream &out;

public:
  explicit Projection(std::ostream &stream) : out(stream) {}
  void q(std::string_view s) {
    out << '"';
    for (unsigned char c : s) {
      if (c == '"' || c == '\\')
        out << '\\' << char(c);
      else if (c < 32) {
        const char *h = "0123456789abcdef";
        out << "\\u00" << h[c >> 4] << h[c & 15];
      } else
        out << char(c);
    }
    out << '"';
  }
  std::string number(std::int64_t n) { return std::to_string(n); }
  std::string number(std::string_view s) {
    auto p = s.find_first_not_of('0');
    return p == s.npos ? "0" : std::string(s.substr(p));
  }
  void n(auto x) { out << number(x); }
  void strings(const auto &v) {
    out << '[';
    bool f = true;
    for (auto x : v) {
      if (!f)
        out << ',';
      f = false;
      q(x);
    }
    out << ']';
  }

  std::string value(const ast::Value *v) {
    if (!v)
      return {};
    std::string s(v->text);
    if (v->quoted)
      s = '"' + s + '"';
    if (!v->file.empty())
      s += " (" + std::string(v->file) + ")";
    return s;
  }
  void field(const ast::Value *v) {
    q(v ? v->text : std::string_view());
    out << ',';
    if (v && !v->file.empty())
      q(v->file);
    else
      out << "null";
  }
  void stmt(const ast::Statement *s) {
    out << '[';
    if (auto *x = cursiv::ast_cast<ast::ReadItemStmt>(s)) {
      q("read_item");
      out << ',';
      q(x->file);
    } else if (auto *x = cursiv::ast_cast<ast::WriteItemStmt>(s)) {
      q("write_item");
      out << ',';
      q(x->file);
    } else if (auto *x = cursiv::ast_cast<ast::TransferStmt>(s)) {
      q("transfer");
      out << ',';
      q(x->from);
      out << ',';
      q(x->to);
    } else if (auto *x = cursiv::ast_cast<ast::CompareStmt>(s)) {
      q("compare");
      out << ',';
      q(x->leftField);
      out << ',';
      q(x->leftFile);
      out << ',';
      q(x->rightField);
      out << ',';
      q(x->rightFile);
    } else if (auto *x = cursiv::ast_cast<ast::IfStmt>(s)) {
      if (x->cond == ast::Cond::end_of_data) {
        q("if_eof");
      } else {
        q("if");
        out << ',';
        q(x->cond == ast::Cond::equal     ? "equal"
          : x->cond == ast::Cond::greater ? "greater"
                                          : "less");
      }
      out << ',';
      n(x->target);
    } else if (auto *x = cursiv::ast_cast<ast::OtherwiseStmt>(s)) {
      q("goto");
      out << ',';
      n(x->target);
    } else if (auto *x = cursiv::ast_cast<ast::JumpToStmt>(s)) {
      q("goto");
      out << ',';
      n(x->target);
    } else if (auto *x = cursiv::ast_cast<ast::MoveStmt>(s)) {
      q("move");
      out << ',';

      q(value(x->source));
      out << ',';
      q(x->destination ? x->destination->text : std::string_view());
      out << ',';
      q(x->destination ? x->destination->file : std::string_view());
    } else if (auto *x = cursiv::ast_cast<ast::SetStmt>(s)) {
      q("set_operation");
      out << ',';
      n(x->which);
      out << ',';
      n(x->target);
    } else if (auto *x = cursiv::ast_cast<ast::TestStmt>(s)) {
      q("test");
      out << ',';
      q(x->field);
      out << ',';
      q(x->file);
      out << ',';

      q(value(x->against));
    } else if (auto *x = cursiv::ast_cast<ast::RewindStmt>(s)) {
      q("rewind");
      out << ',';
      q(x->file);
    } else if (auto *x = cursiv::ast_cast<ast::CloseOutStmt>(s)) {
      q("close_out");
      out << ',';
      strings(x->files);
    } else if (cursiv::ast_cast<ast::StopStmt>(s))
      q("stop");

    else if (auto *x = cursiv::ast_cast<ast::PrintItemStmt>(s)) {
      q("print_item");
      out << ',';
      q(x->file);
    } else if (auto *x = cursiv::ast_cast<ast::ExecuteStmt>(s)) {
      q("execute");
      out << ',';
      n(x->first);
      out << ',';
      n(x->last.empty() ? x->first : x->last);
    } else if (auto *x = cursiv::ast_cast<ast::ArithmeticStmt>(s)) {
      const bool add = x->op == ast::ArithmeticOp::add;
      q(add ? (x->result ? "add_giving" : "add")
        : x->op == ast::ArithmeticOp::subtract ? "subtract"
        : x->op == ast::ArithmeticOp::multiply ? "multiply"
                                               : "divide");
      out << ',';
      field(x->left);
      out << ',';
      field(x->right);
      if (x->result) {
        out << ',';
        field(x->result);
      }
    }
    else
      q("unknown");
    out << ']';
  }
  void tree(const ast::Program *p) {
    if (!p) {
      out << "null";
      return;
    }
    out << '[';
    bool l = true;
    for (auto *x : p->lines) {
      if (!l)
        out << ',';
      l = false;
      out << '[';
      q(number(x->label));
      out << ",[";
      bool first = true;
      for (auto *s : x->statements) {
        const auto file = [&](auto *a, std::string_view mode) {
          for (auto *b : a->files) {
            if (!first)
              out << ',';
            first = false;
            out << '[';
            q(mode);
            out << ',';
            q(b->filename);
            out << ',';
            auto v = b->fileletter;
            if (v.starts_with("FILE-"))
              v.remove_prefix(5);
            q(v);
            out << ']';
          }
        };
        if (auto *a = cursiv::ast_cast<ast::InputStmt>(s)) {
          file(a, "input");
          continue;
        }
        if (auto *a = cursiv::ast_cast<ast::OutputStmt>(s)) {
          file(a, "output");
          continue;
        }
        if (auto *a = cursiv::ast_cast<ast::HspStmt>(s)) {
          if (!first)
            out << ',';
          first = false;
          out << "[\"hsp\",\"HIGH-SPEED-PRINTER\",";
          q(a->file);
          out << ']';
          continue;
        }
        if (!first)
          out << ',';
        first = false;
        stmt(s);
      }
      out << "]]";
    }
    out << ']';
  }
};
} // namespace native_regressions
