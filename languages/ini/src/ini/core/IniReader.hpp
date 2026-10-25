#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "ini/core/ast.hpp"

namespace ini {

std::string source_text(const ast::Line& line);

struct Entry {
  std::string_view key;
  std::string value;
  bool hasValue = false;
  const ast::Line* source = nullptr;
  std::vector<const ast::Line*> continuations;
};

struct Section {
  std::string_view name;
  const ast::Line* source = nullptr;
  std::vector<Entry> entries;
};

struct ReadIssue {
  const ast::Line* source;
  std::string_view message;
};

struct ReadResult {
  std::vector<Section> sections;
  std::vector<ReadIssue> issues;
};

class IniReader final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  ReadResult read(const ast::Document& document) const;
  void checkDocument(const ast::Document& document,
                     cursiv::ValidationAcceptor& accept) const;
};

struct IniCoreServices;
void install_reader(IniCoreServices& services);

}  // namespace ini
