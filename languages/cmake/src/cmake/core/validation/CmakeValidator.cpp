#include "cmake/core/validation/CmakeValidator.hpp"

#include <memory>
#include <string>
#include <vector>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "cursiv/core/workspace/Document.hpp"
#include "cmake/core/CoreServices.hpp"

namespace cmake {
namespace {
std::string lower(std::string_view text) {
  std::string out(text);
  for (char& c : out) if (c >= 'A' && c <= 'Z') c += 'a' - 'A';
  return out;
}

bool command_line(std::string_view gap, bool newline) {
  for (std::size_t i = 0; i < gap.size(); ++i) {
    if (gap[i] == '\n') newline = true;
    if (gap[i] != '#') continue;
    std::size_t end = i + 2;
    if (i + 1 < gap.size() && gap[i + 1] == '[')
      while (end < gap.size() && gap[end] == '=') ++end;
    if (i + 1 == gap.size() || gap[i + 1] != '[' ||
        end == gap.size() || gap[end] != '[') {
      const auto line = gap.find('\n', i);
      if (line == gap.npos) break;
      i = line;
      newline = true;
      continue;
    }
    const std::string close = "]" + std::string(end - i - 2, '=') + "]";
    const auto at = gap.find(close, end + 1);
    if (at == gap.npos) break;
    i = at + close.size() - 1;
    newline = false;
  }
  return newline;
}
}  // namespace

void CmakeValidator::checkArgument(const ast::Argument& argument,
                                   cursiv::ValidationAcceptor& accept) const {
  std::string_view text;
  if (const auto* value = cursiv::ast_cast<ast::UnquotedArgument>(&argument)) {
    text = value->text;
    auto source = text;
    if (const auto* document = argument.getDocument();
        document && argument.cstNodeId != cursiv::kNoNode) {
      const auto& cst = document->parseResult.cst();
      source = cst.text().substr(cst[argument.cstNodeId].begin);
    }
    if (source.starts_with('[')) {
      std::size_t end = 1;
      while (end < source.size() && source[end] == '=') ++end;
      if (end < source.size() && source[end] == '[')
        accept.error(argument, "Bracket argument has no matching closing delimiter.",
                     "cmake.bracket");
    }
  } else if (const auto* value = cursiv::ast_cast<ast::QuotedArgument>(&argument))
    text = value->text;
  else return;
  for (std::size_t i = 0; i < text.size(); ++i) {
    if (text[i] != '\\' || ++i == text.size()) continue;
    const char c = text[i];
    if (((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
         (c >= '0' && c <= '9')) && c != 't' && c != 'r' && c != 'n')
      accept.error(argument, "Invalid character escape: \\" + std::string(1, c),
                   "cmake.escape");
  }
}

void CmakeValidator::checkFile(const ast::File& file,
                               cursiv::ValidationAcceptor& accept) const {
  const auto* document = file.getDocument();
  const auto* cst = document ? &document->parseResult.cst() : nullptr;
  std::size_t previous = 0;
  struct Block { std::string name; const ast::CommandInvocation* command; bool seenElse = false; };
  std::vector<Block> blocks;
  for (const auto* command : file.commands) {
    if (!command) continue;
    if (cst && command->cstNodeId != cursiv::kNoNode) {
      const auto& span = (*cst)[command->cstNodeId];
      const auto source = cst->text();
      if (span.begin >= previous &&
          !command_line(source.substr(previous, span.begin - previous), previous == 0))
        accept.error(*command, "A command must start on a new line.", "cmake.command-line");
      const auto text = cst->text(command->cstNodeId);
      const auto open = text.find('(');
      if (open != text.npos && open >= command->name.size() &&
          text.substr(command->name.size(), open - command->name.size()).find_first_not_of(" \t\r") != text.npos)
        accept.error(*command, "Only spaces may separate a command name from '('.", "cmake.command-open");
      previous = span.end;
    }
    const auto name = lower(command->name);
    if (name == "if" || name == "while" || name == "foreach" ||
        name == "function" || name == "macro" || name == "block")
      blocks.push_back({name, command});
    else if (name == "else" || name == "elseif") {
      if (blocks.empty() || blocks.back().name != "if" || blocks.back().seenElse)
        accept.error(*command, "Unexpected " + name + "().", "cmake.nesting");
      else if (name == "else") blocks.back().seenElse = true;
    } else if (name == "endif" || name == "endwhile" || name == "endforeach" ||
               name == "endfunction" || name == "endmacro" || name == "endblock") {
      if (blocks.empty() || blocks.back().name != name.substr(3))
        accept.error(*command, "Unexpected " + name + "().", "cmake.nesting");
      else blocks.pop_back();
    }
  }
  for (const auto& block : blocks)
    accept.error(*block.command, "Missing end" + block.name + "().", "cmake.nesting");
}

void register_validation_checks(CmakeCoreServices& services) {
  using V = CmakeValidator;
  services.validator = std::make_unique<const CmakeValidator>(services);
  const auto& validator = *services.validator;
  services.validationRegistry->registerChecks({
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkArgument>(validator),
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkFile>(validator)});
}

}  // namespace cmake
