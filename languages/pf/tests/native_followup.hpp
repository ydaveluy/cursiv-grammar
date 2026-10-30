#pragma once
#include "cursiv/test/ValidationHelper.hpp"
#include "cursiv/test/ValidationResult.hpp"

namespace pf_followup {
inline bool run(const pf::PfCoreServices& services) {
  const auto fail = [](int line) { std::fprintf(stderr, "pf follow-up assertion %d\n", line); return false; };
  const auto dump = [](const cursiv::ParseResult& result) {
    if (result.accepted && !result.cst().recovered() && result.diagnostics.empty()) return;
    const auto source = result.cst().text();
    std::fprintf(stderr, "pf follow-up: accepted=%d recovered=%d source=%.*s\n",
                 result.accepted, result.cst().recovered(), int(source.size()), source.data());
    for (const auto& d : result.diagnostics)
      std::fprintf(stderr, "kind=%u begin=%zu end=%zu element=%u\n",
                   unsigned(d.kind), std::size_t(d.begin), std::size_t(d.end), unsigned(d.element));
  };
  const auto result = services.parser->parse(
      "pass in on 'em0' from host.example.org to any set tos 0x2e\n"
      "queue root on em0 bandwidth 1.5M, min 0.5M, max 2M\n"
      "include \"/etc/a\\\"b.conf\"\n");
  dump(result);
  const auto* file = result.value_as<pf::ast::File>();
  const auto* rule = file && file->lines.size() == 3
      ? std::get_if<pf::ast::Rule*>(&file->lines[0]) : nullptr;
  const auto* queue = file && file->lines.size() == 3
      ? std::get_if<pf::ast::QueueRule*>(&file->lines[1]) : nullptr;
  const auto* include = file && file->lines.size() == 3
      ? std::get_if<pf::ast::Include*>(&file->lines[2]) : nullptr;
  if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty() ||
      !rule || !*rule || !(*rule)->onIf || (*rule)->onIf->ifaces.size() != 1 ||
      (*rule)->onIf->ifaces[0]->name != "em0" || !(*rule)->hosts ||
      !(*rule)->hosts->from || (*rule)->hosts->from->hosts.size() != 1 ||
      (*rule)->hosts->from->hosts[0]->address != "host.example.org" ||
      (*rule)->filterOpts.size() != 1 || !(*rule)->filterOpts[0]->viaSet ||
      (*rule)->filterOpts[0]->tos != "0x2e" ||
      !queue || !*queue || (*queue)->opts.size() != 3 ||
      !(*queue)->opts[0]->bw || (*queue)->opts[0]->bw->value != 1.5 ||
      (*queue)->opts[0]->bw->unit != "M" || !(*queue)->opts[1]->bw ||
      (*queue)->opts[1]->bw->value != 0.5 ||
      !include || !*include || (*include)->file != "/etc/a\"b.conf" ||
      (*queue)->opts[0]->bw->parent != (*queue)->opts[0])
    return fail(__LINE__);
  {
    const auto extra = services.parser->parse(
        "pass proto tcp flags any\n"
        "pass proto tcp flags /S\n"
        "pass route-to (em1 192.0.2.1) weight 2 round-robin\n"
        "anchor a in quick on em0\n"
        "set timeout { tcp.first 30 tcp.opening 20 }\n"
        "pass proto tcp from any os { Linux FreeBSD } to any\n"
        "pass proto icmp from any to any icmp-type { 1 2 }\n");
    dump(extra);
    const auto* root = extra.value_as<pf::ast::File>();
    if (!extra.accepted || extra.cst().recovered() || !extra.diagnostics.empty() ||
        !root || root->lines.size() != 7) return fail(__LINE__);
    const auto* a = std::get_if<pf::ast::Rule*>(&root->lines[0]);
    const auto* b = std::get_if<pf::ast::Rule*>(&root->lines[1]);
    const auto* c = std::get_if<pf::ast::Rule*>(&root->lines[2]);
    const auto* anchor = std::get_if<pf::ast::AnchorRule*>(&root->lines[3]);
    if (!a || !*a || (*a)->filterOpts.size() != 1 || !(*a)->filterOpts[0]->flagsAny ||
        (*a)->filterOpts[0]->hasFlagMask || !b || !*b || (*b)->filterOpts.size() != 1 ||
        !(*b)->filterOpts[0]->hasFlagMask || !(*b)->filterOpts[0]->flags1.empty() ||
        (*b)->filterOpts[0]->flags2 != "S" || !c || !*c || (*c)->filterOpts.size() != 1 ||
        (*c)->filterOpts[0]->targets.size() != 1 ||
        (*c)->filterOpts[0]->targets[0]->routeInterface != "em1" ||
        (*c)->filterOpts[0]->targets[0]->address != "192.0.2.1" ||
        (*c)->filterOpts[0]->targets[0]->weight != 2 || !anchor || !*anchor || !(*anchor)->quick)
      return fail(__LINE__);
  }
  for (const auto* invalid : {"match all scrub (no-df", "match all scrub (min-ttl 5"}) {
    const auto bad = services.parser->parse(invalid);
    if (bad.accepted && !bad.cst().recovered() && bad.diagnostics.empty()) return fail(__LINE__);
  }
  for (const auto* source : {"}", "anchor \"open\" {\npass\n", "set", "set optimization"}) {
    const auto validation = cursiv::test::validationHelper(services, source);
    if (validation.diagnostics.empty()) return fail(__LINE__);
  }
  const auto balanced = cursiv::test::validationHelper(
      services, "anchor \"a\" {\nanchor \"b\" {\npass\n}\n}\n");
  return balanced.diagnostics.empty();
}
} // namespace pf_followup
