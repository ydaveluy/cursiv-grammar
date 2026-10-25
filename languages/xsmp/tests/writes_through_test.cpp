// The interpreted writer against the drive over whole catalogues: the same
// records, and every position of several candidates decided as the drive
// decides it.

#include <cstdint>
#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

#include "cursiv/core/syntax-tree/Cst.hpp"
#include "cursiv/core/syntax-tree/CstBuilder.hpp"
#include "cursiv/core/syntax-tree/ParseDiagnostic.hpp"
#include "cursiv/engines/lr/runtime/LrConfig.hpp"
#include "cursiv/engines/lr/runtime/LrContext.hpp"
#include "cursiv/engines/lr/runtime/LrParseStats.hpp"
#include "cursiv/engines/lr/runtime/LrParser.hpp"
#include "cursiv/engines/lr/runtime/LrRepairBudget.hpp"
#include "cursiv/engines/lr/runtime/LrSearch.hpp"
#include "cursiv/engines/lr/runtime/lr_lookahead.hpp"
#include "cursiv/engines/lr/runtime/lr_repair_write.hpp"
#include "cursiv/engines/lr/runtime/lr_search.hpp"
#include "cursiv/test/Check.hpp"
#include "xsmp/generated/Xsmp/Parser.hpp"
#include "xsmp/generated/Xsmp/Table.hpp"

namespace {

namespace gx = ::xsmp::generated::Xsmp;

// The writer's pool over a whole document: the lexing is the drive's --
// trivia laid, the cursor past it -- and a cell is decided as the drive
// decides it.
template <const auto& A>
struct Whole {
  cursiv::LrContext& c;
  cursiv::LrWriteReading& w;
  std::uint32_t forks = 0;
  bool split = false;
  cursiv::LrWriteReading scratch{};

  cursiv::LrWriteReading& operator[](std::size_t i) {
    return i == 0 ? w : scratch;
  }
  std::size_t fork(std::size_t) {
    ++forks;
    return 1;
  }
  void drop(std::size_t) {}
  bool apply(std::size_t i, std::uint32_t at) {
    if (i != 0) return true;
    if (forks != 0) {
      const unsigned arm =
          c.resolveAt(c, c.stack.top().state, at, forks + 1);
      split = split || arm > forks;
      if (arm > forks) return false;
      at += arm;
      forks = 0;
    }
    return cursiv::lr_write_apply<A>(c, w, at);
  }
  bool reduce(std::size_t, std::uint16_t p) {
    return cursiv::lr_write_reduce<A>(c, p, cursiv::lr_shadow_holds(c, w));
  }
  cursiv::Token scan(std::size_t, std::uint16_t mode) {
    c.pending.clear();
    cursiv::TextOffset p = c.committed;
    for (;;) {
      while (p < c.len && cursiv::lr_skips<A>(
                              mode, static_cast<unsigned char>(c.src[p])))
        ++p;
      c.pendEnd = p;
      if (p >= c.len) return cursiv::Token{p, p, cursiv::kEofSymbol};
      const cursiv::Scan s = cursiv::lr_match_text<A>(c.src, c.len, mode, p);
      if (s.term == cursiv::kInvalidSymbol || s.end == p)
        return cursiv::Token{p, p, cursiv::kInvalidSymbol};
      if (!cursiv::lr_is_trivia<A>(s.term) && !cursiv::lr_joins<A>(c, s.term))
        return cursiv::Token{p, s.end, s.term};
      if (A.cfg.terms[s.term].recorded)
        c.pending.push_back(cursiv::Token{p, s.end, s.term});
      p = s.end;
    }
  }
  cursiv::Scan* room() { return c.cand; }
  void lexed() {}
  void kept_at(std::size_t) {}
  void cropped() {}
  void stepped() {}
};

// The drive read off the table: the interpreted writer takes the actions the
// generated states take, lexes as they lex, and decides a cell and a position
// of several candidates as they decide them -- stopping where they stop for
// the graph.
template <const auto& A>
struct Interpreted {
  static inline cursiv::LrDrive drive{};

  struct Pool : Whole<A> {
    void sync() {
      this->c.la = this->w.la;
      this->c.laMode = this->w.laMode;
      this->c.virt = this->w.virt;
      this->c.indent = this->w.indent;
    }
    bool apply(std::size_t i, std::uint32_t at) {
      if (i != 0) return true;
      if (this->forks != 0) {
        sync();
        const unsigned arm = this->c.resolveAt(
            this->c, this->c.stack.top().state, at, this->forks + 1);
        // Past every arm, the drive's switch stops: the graph reads the cell.
        if (arm > this->forks) {
          this->forks = 0;
          return false;
        }
        at += arm;
        this->forks = 0;
      }
      return cursiv::lr_write_apply<A>(this->c, this->w, at);
    }
    void pick(std::size_t, std::uint32_t state, cursiv::Scan* room,
              std::uint32_t n) {
      const cursiv::Scan best = room[0];
      std::uint32_t kept = 0;
      for (std::uint32_t k = 0; k < n; ++k)
        if (A.cold.follow[state].test(room[k].term)) room[kept++] = room[k];
      if (kept == 0) {
        this->w.la = cursiv::Token{this->w.la.begin, best.end, best.term};
        return;
      }
      sync();
      std::vector<cursiv::Scan> arms(room, room + kept);
      const std::uint32_t k =
          kept > 1 ? this->c.candAt(this->c, state, arms.data(), kept) : 0;
      if (k >= kept) {
        this->w.la = cursiv::Token{this->w.la.begin, this->w.la.begin,
                                   cursiv::kInvalidSymbol};
        return;
      }
      this->w.la = cursiv::Token{this->w.la.begin, arms[k].end, arms[k].term};
    }
  };

  static cursiv::LrExit enter(cursiv::LrContext& c) {
    cursiv::LrWriteReading w;
    w.run.s = &c.stack;
    w.at = c.committed;
    w.la = c.la;
    w.laMode = c.laMode;
    w.virt = c.virt;
    w.indent = c.indent;
    Pool x{{c, w}};
    for (;;) {
      w.dead = false;
      cursiv::lr_read_step<A>(c, x, 0);
      x.sync();
      if (w.dead) return cursiv::LrExit::pause;
      if (w.done) {
        cursiv::lr_flush_trivia<A>(c);
        return cursiv::LrExit::accept;
      }
    }
  }
};

// A valid document parsed twice, by the drive and by the interpreted
// writer, graphs and all: the records are the same. The graphs the drive's
// parse opened, counted.
template <const auto& A>
unsigned writes_through(const cursiv::LrDrive& drive, std::string_view text,
                        const char* name) {
  Interpreted<A>::drive = drive;
  Interpreted<A>::drive.enter = &Interpreted<A>::enter;
  cursiv::Cst ran;
  cursiv::LrParseStats stats{};
  const bool ok = cursiv::lr_parser<A>.parse(text, ran, &stats, &drive);
  cursiv::Cst read;
  const bool same =
      cursiv::lr_parser<A>.parse(text, read, nullptr, &Interpreted<A>::drive);
  if (!ok || !same || !ran.diagnostics().empty() ||
      !read.diagnostics().empty()) {
    std::printf("FAIL: %s: accepted %d by the drive, %d interpreted\n", name,
                ok ? 1 : 0, same ? 1 : 0);
    ++failures;
    return stats.splits;
  }
  if (ran.size() != read.size()) {
    std::printf("FAIL: %s: %u records interpreted, the drive's %u\n", name,
                read.size(), ran.size());
    ++failures;
    return stats.splits;
  }
  for (cursiv::NodeCount i = 0; i < ran.size(); ++i) {
    const cursiv::CstNode& x0 = ran[i];
    const cursiv::CstNode& y0 = read[i];
    if (x0.begin == y0.begin && x0.end == y0.end &&
        x0.subtreeSize == y0.subtreeSize && x0.engine == y0.engine &&
        x0.hidden == y0.hidden && x0.recovered == y0.recovered)
      continue;
    std::printf("FAIL: %s: record %u differs from the drive's\n", name, i);
    ++failures;
    return stats.splits;
  }
  return stats.splits;
}

}  // namespace

int main() {
  // Whole valid documents written by the interpreted path: xsmp opens no
  // graph, its positions of several candidates are decided as the drive
  // decides them.
  for (const char* f : {"ecss_smp_l1.xsmpcat", "example/demo.xsmpcat"})
    (void)writes_through<gx::kTable>(
        gx::kDrive,
        cursiv::test::read_file(std::string(CURSIV_EXAMPLE_DIR) + "/" + f), f);
  return cursiv::test::report("xsmp_writes_through_test");
}
