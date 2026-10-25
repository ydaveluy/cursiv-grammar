// The grammar's smoke test: every document under example/ that is not
// expected to fail parses clean, and a few real field values read back
// correctly -- a converter that silently gave the wrong value would still
// pass a structural-only check.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "arden/core/CoreServices.hpp"
#include "arden/core/ast.hpp"

namespace {

int fail(const std::string& what) {
  std::fprintf(stderr, "arden: %s\n", what.c_str());
  return 1;
}

template <class T>
const T* as(const cursiv::AstNode* node) {
  if (node == nullptr || node->type != &cursiv::ast_type_tag<T>) return nullptr;
  return static_cast<const T*>(node);
}

std::string read_file(const std::filesystem::path& path) {
  std::ifstream file(path);
  return std::string((std::istreambuf_iterator<char>(file)),
                     std::istreambuf_iterator<char>());
}

}  // namespace

int main() {
  const auto language = cursiv::create_language<
      arden::ArdenCoreServices, arden::ArdenSharedCoreServices,
      arden::install_core_module>();

  std::vector<std::filesystem::path> paths;
  for (const auto& entry :
       std::filesystem::directory_iterator(CURSIV_EXAMPLE_DIR))
    if (entry.is_regular_file()) paths.push_back(entry.path());
  if (paths.empty()) return fail("no example documents found");

  // Every document's text stays alive for the whole run: every string_view
  // in a tree points into it.
  std::vector<std::string> texts;
  texts.reserve(paths.size());
  std::vector<cursiv::ParseResult> results;
  results.reserve(paths.size());

  for (const auto& path : paths) {
    texts.push_back(read_file(path));
    cursiv::ParseResult result = language.language.parser->parse(texts.back());
    if (result.cst().recovered())
      return fail(path.string() + ": the document does not parse");
    if (!result.diagnostics.empty())
      return fail(path.string() + ": unexpected diagnostics");
    if (result.value == nullptr ||
        result.value->type != &cursiv::ast_type_tag<arden::ast::Mlm>)
      return fail(path.string() + ": the parse built no mlm");
    results.push_back(std::move(result));
  }

  // "Contraindication Alert.mlm": read real field values back out, so a
  // converter that silently mis-decoded one would still fail here.
  const arden::ast::Mlm* alert = nullptr;
  for (std::size_t i = 0; i < paths.size(); ++i)
    if (paths[i].filename() == "Contraindication Alert.mlm")
      alert = static_cast<const arden::ast::Mlm*>(results[i].value);
  if (alert == nullptr) return fail("Contraindication Alert.mlm was not found");

  if (alert->maintenance == nullptr ||
      alert->maintenance->title != "Check for penicillin allergy")
    return fail("the title slot did not read back");
  if (alert->maintenance->mlmName != "pen_allergy")
    return fail("the mlmname slot did not read back");
  if (alert->maintenance->validation != "testing")
    return fail("the validation slot did not read back");
  if (alert->maintenance->date != "1991-03-18")
    return fail("the date slot did not read back");

  if (alert->library == nullptr || alert->library->keywords != "penicillin; allergy")
    return fail("the keywords slot did not read back");

  if (alert->knowledge == nullptr || alert->knowledge->type != "data-driven")
    return fail("the type slot did not read back");
  if (alert->knowledge->dataStatements.size() != 2)
    return fail("the data slot did not hold two statements");
  // `penicillin_order := event {medication_order where class = penicillin};`
  const auto* first_assign =
      as<arden::ast::IdentReadAssignStmt>(alert->knowledge->dataStatements[0]);
  if (first_assign == nullptr)
    return fail("the first data statement is not an ident-read assignment");
  if (first_assign->target != "penicillin_order")
    return fail("the first data statement's target did not read back");
  const auto* mapping = as<arden::ast::MappingExpr>(first_assign->value);
  if (mapping == nullptr || mapping->kind != "event")
    return fail("the first data statement's mapping did not read back");

  if (alert->knowledge->urgency == nullptr)
    return fail("the urgency slot is missing");
  const auto* urgency_number =
      as<arden::ast::NumberLit>(alert->knowledge->urgency);
  if (urgency_number == nullptr || urgency_number->value != 50.0)
    return fail("the urgency value did not read back");

  if (alert->resources == nullptr || alert->resources->defaultLanguage != "en")
    return fail("the default language slot did not read back");
  if (alert->resources->languages.size() != 1 ||
      alert->resources->languages[0]->code != "en")
    return fail("the language slot did not read back");

  // self_reference.mlm: this port's own corpus, exercising the mlm-self
  // reference -- no upstream example uses it.
  {
    const std::string text =
        read_file(std::string(CURSIV_TEST_DIR) + "/self_reference.mlm");
    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered())
      return fail("self_reference.mlm does not parse");
    if (!result.diagnostics.empty())
      return fail("self_reference.mlm carries a diagnostic");
    const auto* mlm = as<arden::ast::Mlm>(result.value);
    if (mlm == nullptr || mlm->knowledge == nullptr ||
        mlm->knowledge->dataStatements.size() != 2)
      return fail("self_reference.mlm: wrong data statement count");
    const auto* self_stmt =
        as<arden::ast::IdentReadAssignStmt>(mlm->knowledge->dataStatements[0]);
    const auto* self_ref =
        self_stmt != nullptr ? as<arden::ast::MlmRefExpr>(self_stmt->value)
                             : nullptr;
    if (self_ref == nullptr || !self_ref->self)
      return fail("self_reference.mlm: MLM_SELF did not read back");
    const auto* other_stmt =
        as<arden::ast::IdentReadAssignStmt>(mlm->knowledge->dataStatements[1]);
    const auto* other_ref =
        other_stmt != nullptr ? as<arden::ast::MlmRefExpr>(other_stmt->value)
                              : nullptr;
    if (other_ref == nullptr || other_ref->self ||
        other_ref->name != "other_mlm_name" ||
        other_ref->institution != "Test Hospital")
      return fail("self_reference.mlm: the named MLM reference did not read back");
  }

  // Annex A4 precedence probes: a minimal MLM shell around one data-slot
  // expression, read back at the AST level to discriminate the ladder this
  // port now builds from the one it built before the A4 rebuild.
  {
    const char* const shell =
        "maintenance:\n"
        "title: t;;\n"
        "mlmname: m;;\n"
        "arden: version 3;;\n"
        "version: v;;\n"
        "institution: i;;\n"
        "author: a;;\n"
        "specialist: s;;\n"
        "date: 2020-01-01;;\n"
        "validation: testing;;\n"
        "library:\n"
        "purpose: p;;\n"
        "explanation: e;;\n"
        "keywords: k;;\n"
        "knowledge:\n"
        "type: data-driven;;\n"
        "data:\n"
        "var := ";
    const char* const tail =
        ";\n"
        ";;\n"
        "evoke: ;;\n"
        "logic: ;;\n"
        "action: ;;\n"
        "resources:\n"
        "default: en;;\n"
        "language: en;;\n"
        "end:\n";

    // Every probed text stays alive for the whole run: `value` and every
    // node under it point into it.
    std::vector<std::string> probe_texts;
    std::vector<cursiv::ParseResult> probe_results;
    probe_texts.reserve(64);
    probe_results.reserve(64);

    auto probe = [&](const std::string& expr) -> const arden::ast::Expr* {
      probe_texts.push_back(shell + expr + tail);
      probe_results.push_back(
          language.language.parser->parse(probe_texts.back()));
      const cursiv::ParseResult& result = probe_results.back();
      if (result.cst().recovered() || !result.diagnostics.empty()) return nullptr;
      const auto* mlm = as<arden::ast::Mlm>(result.value);
      if (mlm == nullptr || mlm->knowledge == nullptr ||
          mlm->knowledge->dataStatements.empty())
        return nullptr;
      const auto* assign =
          as<arden::ast::AssignStmt>(mlm->knowledge->dataStatements[0]);
      if (assign == nullptr) return nullptr;
      // A bare `expr` is a one-item ExprList (ast.hpp): unwrap it. A
      // trailing `[...]`/`.` instead nests an IndexExpr/FieldExpr directly
      // over the comma list, so `value` is already the probed node.
      const auto* list = as<arden::ast::ExprList>(assign->value);
      if (list != nullptr) return list->items.size() == 1 ? list->items[0] : nullptr;
      return assign->value;
    };
    // A `[...]` step's own base is built through ExprCommaList too, so it
    // carries the same one-item-ExprList wrapping as `probe()` unwraps.
    auto unwrap = [](const arden::ast::Expr* e) -> const arden::ast::Expr* {
      const auto* list = as<arden::ast::ExprList>(e);
      return list != nullptr && list->items.size() == 1 ? list->items[0] : e;
    };

    // seqto and AS bind tightest of all -- beside the named functions and
    // `.`, nowhere near NOT/AND/OR.
    const auto* seqto_add = as<arden::ast::BinaryExpr>(probe("a seqto b + c"));
    if (seqto_add == nullptr || seqto_add->op != arden::ast::BinOp::add ||
        as<arden::ast::FuncApply>(seqto_add->left) == nullptr)
      return fail("seqto is not tighter than binary +");

    const auto* seqto_as = as<arden::ast::FuncApply>(probe("a seqto b as number"));
    if (seqto_as == nullptr || as<arden::ast::CastExpr>(seqto_as->from) == nullptr)
      return fail("as is not tighter than seqto");

    // Unary +/- reads looser than binary +/-, matching HL7's own errata
    // ("... has higher precedence than unary minus"): `-a+b` is `-(a+b)`.
    const auto* neg = as<arden::ast::FuncApply>(probe("-a + b"));
    if (neg == nullptr || neg->func != "-" ||
        as<arden::ast::BinaryExpr>(neg->operand) == nullptr)
      return fail("unary minus does not read looser than binary +");

    // `[...]` (loosest of the postfix pair) applies to the whole sum, not
    // just its right addend: `a+b[1]` is `(a+b)[1]`.
    const auto* idx = as<arden::ast::IndexExpr>(probe("a + b[1]"));
    if (idx == nullptr || as<arden::ast::BinaryExpr>(unwrap(idx->base)) == nullptr)
      return fail("[...] does not apply to the whole sum");

    // `.` (tightest besides seqto/as) binds before `+`: `a+b.c` is
    // `a+(b.c)`, not `(a+b).c`.
    const auto* dot_add = as<arden::ast::BinaryExpr>(probe("a + b.c"));
    if (dot_add == nullptr ||
        as<arden::ast::FieldExpr>(dot_add->right) == nullptr)
      return fail(". does not bind tighter than +");

    // A `[...]` step still admits a trailing `.field` (the corpus's own
    // `namelist[1].FirstName`, `my_obj.message_list[n].msg`): `.` here is
    // the loose group's own continuation, not a second route into the
    // tight one.
    const auto* chained = as<arden::ast::FieldExpr>(probe("x[1].y"));
    if (chained == nullptr || as<arden::ast::IndexExpr>(chained->base) == nullptr)
      return fail("a [...] step does not admit a trailing .field");

    // `matches pattern` moved out of the flat comparison group to the tight
    // cluster near duration suffixes: it now binds tighter than `=`.
    const auto* cmp_matches =
        as<arden::ast::BinaryComparison>(probe("\"x\" matches pattern \"y\" = true"));
    if (cmp_matches == nullptr ||
        as<arden::ast::MatchesPattern>(cmp_matches->left) == nullptr)
      return fail("matches pattern does not bind tighter than =");

    // Factoring the matches head must retain the existing precedence and
    // wider right operand, without the indirect ExprStringPrefix cycle.
    const auto* add_matches =
        as<arden::ast::BinaryExpr>(probe("1 + 2 matches pattern \"x\""));
    if (add_matches == nullptr || add_matches->op != arden::ast::BinOp::add ||
        as<arden::ast::NumberLit>(add_matches->left) == nullptr ||
        as<arden::ast::MatchesPattern>(add_matches->right) == nullptr)
      return fail("matches pattern does not bind to the right addend");

    const auto* matches_add =
        as<arden::ast::MatchesPattern>(probe("a matches pattern b + c"));
    if (matches_add == nullptr ||
        as<arden::ast::BinaryExpr>(matches_add->pattern) == nullptr)
      return fail("matches pattern lost its wider right operand");

    const auto* neg_matches =
        as<arden::ast::FuncApply>(probe("-a + b matches pattern c"));
    const auto* neg_matches_add = neg_matches != nullptr
        ? as<arden::ast::BinaryExpr>(neg_matches->operand) : nullptr;
    if (neg_matches == nullptr || neg_matches->func != "-" ||
        neg_matches_add == nullptr ||
        as<arden::ast::MatchesPattern>(neg_matches_add->right) == nullptr)
      return fail("matches pattern changed unary minus precedence");

    const auto* upper_matches = as<arden::ast::CaseConvExpr>(
        probe("uppercase \"x\" matches pattern \"y\""));
    if (upper_matches == nullptr || !upper_matches->upper ||
        as<arden::ast::MatchesPattern>(upper_matches->value) == nullptr)
      return fail("matches pattern changed uppercase precedence");

    const auto* trim_matches = as<arden::ast::TrimExpr>(
        probe("trim left \"x\" matches pattern \"y\""));
    if (trim_matches == nullptr || trim_matches->side != "left" ||
        as<arden::ast::MatchesPattern>(trim_matches->value) == nullptr)
      return fail("matches pattern changed trim precedence");

    const auto* concat_matches =
        as<arden::ast::BinaryExpr>(probe("a || b matches pattern c"));
    const auto* matches_concat =
        as<arden::ast::BinaryExpr>(probe("a matches pattern b || c"));
    if (concat_matches == nullptr ||
        concat_matches->op != arden::ast::BinOp::concat ||
        as<arden::ast::MatchesPattern>(concat_matches->right) == nullptr ||
        matches_concat == nullptr ||
        matches_concat->op != arden::ast::BinOp::concat ||
        as<arden::ast::MatchesPattern>(matches_concat->left) == nullptr)
      return fail("matches pattern changed concatenation precedence");

    const auto* matches_chain = as<arden::ast::MatchesPattern>(
        probe("a matches pattern b matches pattern c"));
    if (matches_chain == nullptr ||
        as<arden::ast::IdentExpr>(matches_chain->value) == nullptr ||
        as<arden::ast::MatchesPattern>(matches_chain->pattern) == nullptr)
      return fail("matches pattern is not right associative");

    const auto* localized_matches = as<arden::ast::MatchesPattern>(
        probe("localized 'x' matches pattern y"));
    const auto* localized_value = localized_matches != nullptr
        ? as<arden::ast::LocalizedExpr>(localized_matches->value) : nullptr;
    if (localized_value == nullptr || localized_value->term != "x")
      return fail("a localized term cannot be the matched value");

    const auto* parenthesized_prefix = as<arden::ast::MatchesPattern>(
        probe("(uppercase \"x\") matches pattern \"Y\""));
    const auto* parenthesized_case = parenthesized_prefix != nullptr
        ? as<arden::ast::CaseConvExpr>(unwrap(parenthesized_prefix->value))
        : nullptr;
    if (parenthesized_case == nullptr || !parenthesized_case->upper ||
        as<arden::ast::StringLit>(parenthesized_case->value) == nullptr ||
        parenthesized_prefix->value->parent != parenthesized_prefix ||
        parenthesized_case->parent != parenthesized_prefix->value)
      return fail("a parenthesized string prefix lost its matches value or parent");

    const auto* duration_ago = as<arden::ast::FuncApply>(probe("3 days ago"));
    const auto* days = duration_ago != nullptr
        ? as<arden::ast::FuncApply>(duration_ago->operand) : nullptr;
    if (duration_ago == nullptr || duration_ago->func != "ago" ||
        days == nullptr || days->func != "days" ||
        as<arden::ast::NumberLit>(days->operand) == nullptr ||
        days->parent != duration_ago || days->operand->parent != days)
      return fail("ago does not wrap the duration suffix");

    const auto* parenthesized_ago =
        as<arden::ast::FuncApply>(probe("(3 days) ago"));
    const auto* parenthesized_days = parenthesized_ago != nullptr
        ? as<arden::ast::FuncApply>(unwrap(parenthesized_ago->operand)) : nullptr;
    if (parenthesized_ago == nullptr || parenthesized_ago->func != "ago" ||
        parenthesized_days == nullptr || parenthesized_days->func != "days" ||
        parenthesized_ago->operand->parent != parenthesized_ago ||
        parenthesized_days->parent != parenthesized_ago->operand)
      return fail("a parenthesized duration lost its ago operand or parent");

    const auto* ago_matches = as<arden::ast::MatchesPattern>(
        probe("3 days ago matches pattern x"));
    const auto* matched_ago = ago_matches != nullptr
        ? as<arden::ast::FuncApply>(ago_matches->value) : nullptr;
    if (matched_ago == nullptr || matched_ago->func != "ago")
      return fail("an ago expression cannot start another matches pattern");

    const auto* matches_ago =
        as<arden::ast::FuncApply>(probe("x matches pattern y days ago"));
    const auto* ago_match = matches_ago != nullptr
        ? as<arden::ast::MatchesPattern>(matches_ago->operand) : nullptr;
    const auto* pattern_days = ago_match != nullptr
        ? as<arden::ast::FuncApply>(ago_match->pattern) : nullptr;
    if (matches_ago == nullptr || matches_ago->func != "ago" ||
        pattern_days == nullptr || pattern_days->func != "days")
      return fail("ago changed the matches/duration suffix association");

    // The port also accepts type-inconsistent combinations of Boolean
    // matches and duration-only ago. Give both backends one stable shape:
    // the wider pattern argument contains the next ago/matches pair.
    const auto* mixed_matches = as<arden::ast::MatchesPattern>(
        probe("a matches pattern b ago matches pattern y"));
    const auto* mixed_pattern = mixed_matches != nullptr
        ? as<arden::ast::MatchesPattern>(mixed_matches->pattern) : nullptr;
    const auto* mixed_ago = mixed_pattern != nullptr
        ? as<arden::ast::FuncApply>(mixed_pattern->value) : nullptr;
    if (mixed_matches == nullptr ||
        as<arden::ast::IdentExpr>(mixed_matches->value) == nullptr ||
        mixed_ago == nullptr || mixed_ago->func != "ago" ||
        mixed_pattern->parent != mixed_matches ||
        mixed_ago->parent != mixed_pattern)
      return fail("mixed matches/ago chain has an inconsistent shape");

    if (probe("3 days days") != nullptr || probe("x ago ago") != nullptr)
      return fail("a duration or ago suffix repeats without a match");

    // `first n from l` is the function, as the standard's examples
    // (SyntaxTest.mlm) write `MINIMUM 2 FROM (11, 14, 13, 12)`; only
    // parentheses make `(first n)` the left side of a time comparison.
    const auto* first_from = as<arden::ast::FuncApply>(probe("first 2 from l"));
    if (first_from == nullptr || first_from->func != "first" ||
        as<arden::ast::ExprList>(first_from->operand) == nullptr ||
        as<arden::ast::IdentExpr>(first_from->from) == nullptr)
      return fail("first n from l is not the function");

    const auto* index_min =
        as<arden::ast::FuncApply>(probe("index minimum 2 from (11, 14)"));
    if (index_min == nullptr || !index_min->index ||
        as<arden::ast::ExprList>(index_min->operand) == nullptr ||
        as<arden::ast::ExprList>(index_min->from) == nullptr)
      return fail("index minimum n from l is not the function");

    const auto* nested =
        as<arden::ast::FuncApply>(probe("last (first n from l)"));
    if (nested == nullptr || nested->func != "last" ||
        as<arden::ast::ExprList>(nested->operand) == nullptr)
      return fail("last (first n from l) is not nested functions");

    const auto* using_from =
        as<arden::ast::FuncApply>(probe("first 2 from l using it"));
    if (using_from == nullptr || using_from->from == nullptr ||
        as<arden::ast::ItExpr>(using_from->extra) == nullptr)
      return fail("first n from l using it is not one function");

    const auto* using_max =
        as<arden::ast::FuncApply>(probe("maximum l using it"));
    if (using_max == nullptr || as<arden::ast::IdentExpr>(using_max->operand) == nullptr ||
        as<arden::ast::ItExpr>(using_max->extra) == nullptr)
      return fail("maximum l using it is not one function");

    const auto* paren_from =
        as<arden::ast::BinaryComparison>(probe("(first 2) from t"));
    if (paren_from == nullptr ||
        paren_from->op != arden::ast::CompareOp::from_rel ||
        as<arden::ast::ExprList>(paren_from->left) == nullptr)
      return fail("(first n) from t is not the time comparison");

    const auto* duration_from =
        as<arden::ast::BinaryComparison>(probe("3 days from t"));
    if (duration_from == nullptr ||
        duration_from->op != arden::ast::CompareOp::from_rel)
      return fail("3 days from t is not the time comparison");
  }

  std::puts("arden: every example parses");
  return 0;
}
