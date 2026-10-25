#include "llvmir/core/validation/LlvmIrValidator.hpp"

#include <algorithm>
#include <charconv>
#include <functional>
#include <memory>
#include <unordered_set>

#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "llvmir/core/CoreServices.hpp"

namespace llvmir {
namespace {
std::optional<std::uint64_t> unsigned_value(std::string_view text) {
  std::uint64_t n = 0;
  int base = 10;
  if (text.starts_with("u0x")) {
    text.remove_prefix(3);
    base = 16;
  }
  const auto [end, error] =
      std::from_chars(text.data(), text.data() + text.size(), n, base);
  if (error != std::errc{} || end != text.data() + text.size()) return {};
  return n;
}
bool alignment(std::string_view text, std::uint64_t max = 1ULL << 32) {
  const auto n = unsigned_value(text);
  return n && *n && *n <= max && (*n & (*n - 1)) == 0;
}
bool one_of(std::string_view word,
            std::initializer_list<std::string_view> set) {
  return std::find(set.begin(), set.end(), word) != set.end();
}
bool contains_nul(std::string_view text) {
  for (std::size_t i = 0; i < text.size(); ++i) {
    if (text[i] == '\0') return true;
    if (text[i] != '\\' || i + 1 == text.size()) continue;
    if (text[i + 1] == '\\') {
      ++i;
      continue;
    }
    if (i + 2 < text.size() && text[i + 1] == '0' && text[i + 2] == '0')
      return true;
  }
  return false;
}
template <auto Member, class Owner>
void check_name(const Owner& node, std::string_view name,
                cursiv::ValidationAcceptor& accept) {
  if (contains_nul(name))
    accept.error(node, "NUL character is not allowed in names")
        .template property<Member>()
        .code("llvmir.nul-name");
}
template <auto Member, class Owner>
void check_name(const Owner& node, const std::optional<std::string_view>& name,
                cursiv::ValidationAcceptor& accept) {
  if (name) check_name<Member>(node, *name, accept);
}
}  // namespace

void LlvmIrValidator::checkType(const ast::Type& node,
                                cursiv::ValidationAcceptor& accept) const {
  if (const auto* n = cursiv::ast_cast<ast::IntType>(&node)) {
    const auto width = unsigned_value(n->text.substr(1));
    if (!width || *width == 0 || *width > (1 << 23))
      accept.error(node, "integer width must be between 1 and 8388608");
  } else if (const auto* n = cursiv::ast_cast<ast::ByteType>(&node)) {
    const auto width = unsigned_value(n->text.substr(1));
    if (!width || *width == 0 || *width > (1 << 23))
      accept.error(node, "byte width must be between 1 and 8388608");
  } else if (const auto* n = cursiv::ast_cast<ast::ArrayType>(&node)) {
    if (!unsigned_value(n->length))
      accept.error(node, "array length must be a uint64");
  } else if (const auto* n = cursiv::ast_cast<ast::VectorType>(&node)) {
    const auto length = unsigned_value(n->length);
    if (!length || *length == 0 || *length > 0xffffffffULL)
      accept.error(node, "vector length must be a non-zero uint32");
  } else if (const auto* n = cursiv::ast_cast<ast::PointerType>(&node)) {
    if (n->addrSpace) {
      const auto space = unsigned_value(*n->addrSpace);
      if (!space || *space > 0xffffff)
        accept.error(node, "address space must fit in 24 bits");
    }
  } else if (const auto* n = cursiv::ast_cast<ast::TargetType>(&node)) {
    bool seen_integer = false;
    for (const auto& p : n->parameters) {
      if (const auto* integer = std::get_if<std::string_view>(&p)) {
        seen_integer = true;
        const auto value = unsigned_value(*integer);
        if (!value || *value > 0xffffffffULL)
          accept.error(node, "target type integer parameters must be uint32");
      } else if (seen_integer) {
        accept.error(node,
                     "target type parameters must precede integer parameters");
      }
    }
  }
}

void LlvmIrValidator::checkAttribute(const ast::Attr& node,
                                     cursiv::ValidationAcceptor& accept) const {
  if (one_of(node.name, {"align", "alignstack"}) && node.intArg &&
      !alignment(*node.intArg))
    accept.error(node,
                 "alignment must be a power of two no greater than 4294967296");
  if (node.name == "allocsize" && node.intArg2 && node.intArg == node.intArg2)
    accept.error(node, "allocsize indices must refer to different parameters");
  if (node.name == "memory") {
    bool seen_location = false, seen_target_location = false;
    if (node.entries.empty())
      accept.error(node, "memory requires an access kind");
    for (const auto* entry : node.entries) {
      if (entry->grouped || entry->values.size() != 1 ||
          !one_of(entry->values.empty() ? "" : entry->values[0],
                  {"none", "read", "write", "readwrite"}))
        accept.error(*entry,
                     "memory access must be none, read, write or readwrite");
      if (entry->name) {
        if (!one_of(*entry->name, {"argmem", "inaccessiblemem", "errnomem",
                                   "target_mem", "target_mem0", "target_mem1"}))
          accept.error(*entry, "unknown memory location");
        if (*entry->name == "target_mem" && seen_target_location)
          accept.error(
              *entry,
              "target memory default must precede its specific locations");
        seen_target_location |=
            one_of(*entry->name, {"target_mem0", "target_mem1"});
        seen_location = true;
      } else if (seen_location) {
        accept.error(*entry, "default memory access must be specified first");
      }
    }
  }
  if (node.name == "nofpclass") {
    bool valid = node.entries.size() == 1;
    if (valid) {
      const auto& entry = *node.entries[0];
      valid = !entry.name && !entry.grouped && !entry.values.empty();
      if (entry.values.size() == 1 && unsigned_value(entry.values[0])) {
        const auto mask = *unsigned_value(entry.values[0]);
        valid &= mask != 0 && mask <= 1023;
      } else {
        for (const auto word : entry.values)
          valid &= one_of(word, {"all", "nan", "snan", "qnan", "inf", "ninf",
                                 "pinf", "norm", "nnorm", "pnorm", "sub",
                                 "nsub", "psub", "zero", "nzero", "pzero"});
      }
    }
    if (!valid)
      accept.error(node,
                   "nofpclass requires a non-zero 10-bit mask or floating "
                   "point classes");
  }
}

void LlvmIrValidator::checkInstruction(
    const ast::Instruction& node, cursiv::ValidationAcceptor& accept) const {
  if (const auto* n = cursiv::ast_cast<ast::GenericInst>(&node)) {
    if (n->result && one_of(n->op, {"store", "fence", "resume", "unreachable",
                                    "catchret", "cleanupret"}))
      accept.error(node, "a void instruction cannot have a result name");
    for (std::size_t i = 0; i < n->align.size(); ++i)
      if (!alignment(n->align[i]))
        accept.error(*n,
                     "instruction alignment must be a power of two no greater "
                     "than 4294967296")
            .property<&ast::GenericInst::align>(i)
            .code("llvmir.alignment");
    for (std::size_t i = 0; i < n->addrSpace.size(); ++i) {
      const auto space = unsigned_value(n->addrSpace[i]);
      if (!space || *space > 0xffffff)
        accept.error(*n, "address space must fit in 24 bits")
            .property<&ast::GenericInst::addrSpace>(i)
            .code("llvmir.address-space");
    }
    const bool atomic =
        std::find(n->flags.begin(), n->flags.end(), "atomic") != n->flags.end();
    if (atomic && n->align.empty() && one_of(n->op, {"load", "store"}))
      accept.error(node, "atomic load and store require explicit alignment");
    for (const auto order : n->orderings) {
      if ((n->op == "load" && one_of(order, {"release", "acq_rel"})) ||
          (n->op == "store" && one_of(order, {"acquire", "acq_rel"})) ||
          (n->op == "fence" && one_of(order, {"unordered", "monotonic"})) ||
          (one_of(n->op, {"atomicrmw", "cmpxchg"}) && order == "unordered"))
        accept.error(node, "atomic ordering is invalid for this instruction");
    }
    if (n->op == "cmpxchg" && n->orderings.size() == 2 &&
        one_of(n->orderings[1], {"release", "acq_rel"}))
      accept.error(node, "cmpxchg failure ordering cannot release");
    if (n->op == "atomicrmw" &&
        std::find(n->flags.begin(), n->flags.end(), "elementwise") !=
            n->flags.end() &&
        std::find(n->orderings.begin(), n->orderings.end(), "seq_cst") !=
            n->orderings.end())
      accept.error(node,
                   "elementwise atomicrmw cannot be sequentially consistent");
    for (const auto flag : n->flags) {
      if ((flag == "disjoint" && n->op != "or") ||
          (flag == "nneg" && !one_of(n->op, {"zext", "uitofp"})) ||
          (one_of(flag, {"nuw", "nsw"}) &&
           !one_of(n->op,
                   {"add", "sub", "mul", "shl", "trunc", "getelementptr"})))
        accept.error(node, "flag is not valid for this instruction");
    }
  } else if (const auto* n = cursiv::ast_cast<ast::CallInst>(&node)) {
    if (n->result && n->type && cursiv::ast_cast<ast::VoidType>(n->type))
      accept.error(node, "a void call cannot have a result name");
  } else if (node.result && (cursiv::ast_cast<ast::RetInst>(&node) ||
                             cursiv::ast_cast<ast::BrInst>(&node) ||
                             cursiv::ast_cast<ast::CondBrInst>(&node) ||
                             cursiv::ast_cast<ast::SwitchInst>(&node) ||
                             cursiv::ast_cast<ast::IndirectBrInst>(&node))) {
    accept.error(node, "a terminator cannot have a result name");
  }
}

void LlvmIrValidator::checkMetadata(const ast::SpecializedMdNode& node,
                                    cursiv::ValidationAcceptor& accept) const {
  std::unordered_set<std::string_view> names;
  for (const auto* field : node.fields)
    if (!names.insert(field->name).second)
      accept.error(*field, "duplicate metadata field");
  if ((node.kind == "!DIExpression" && !node.fields.empty()) ||
      (node.kind != "!DIExpression" && !node.positional.empty()))
    accept.error(node, "only DIExpression uses positional metadata fields");
}

void LlvmIrValidator::checkModule(const ast::Module& node,
                                  cursiv::ValidationAcceptor& accept) const {
  std::unordered_set<std::uint64_t> definitions;
  for (const auto* entity : node.entities) {
    if (const auto* def = cursiv::ast_cast<ast::MetadataDef>(entity)) {
      const auto id = unsigned_value(def->id.substr(1));
      if (!id || *id > 0xffffffffULL)
        accept.error(*def, "metadata identifier must be a uint32");
      if (id && !definitions.insert(*id).second)
        accept.error(*def, "duplicate metadata definition");
    }
  }
  std::function<void(const cursiv::AstNode&)> visit = [&](const auto& n) {
    if (const auto* name = cursiv::ast_cast<ast::Ident>(&n))
      check_name<&ast::Ident::name>(*name, name->name, accept);
    if (const auto* name = cursiv::ast_cast<ast::FuncHeader>(&n))
      check_name<&ast::FuncHeader::name>(*name, name->name, accept);
    if (const auto* name = cursiv::ast_cast<ast::GlobalVar>(&n))
      check_name<&ast::GlobalVar::name>(*name, name->name, accept);
    if (const auto* name = cursiv::ast_cast<ast::IndirectSymbol>(&n))
      check_name<&ast::IndirectSymbol::name>(*name, name->name, accept);
    if (const auto* name = cursiv::ast_cast<ast::TypeDef>(&n))
      check_name<&ast::TypeDef::name>(*name, name->name, accept);
    if (const auto* name = cursiv::ast_cast<ast::NamedType>(&n))
      check_name<&ast::NamedType::name>(*name, name->name, accept);
    if (const auto* name = cursiv::ast_cast<ast::ComdatDef>(&n))
      check_name<&ast::ComdatDef::name>(*name, name->name, accept);
    if (const auto* name = cursiv::ast_cast<ast::Param>(&n))
      check_name<&ast::Param::name>(*name, name->name, accept);
    if (const auto* name = cursiv::ast_cast<ast::BasicBlock>(&n))
      check_name<&ast::BasicBlock::label>(*name, name->label, accept);
    if (const auto* name = cursiv::ast_cast<ast::Label>(&n))
      check_name<&ast::Label::name>(*name, name->name, accept);
    if (const auto* name = cursiv::ast_cast<ast::PhiIncoming>(&n))
      check_name<&ast::PhiIncoming::block>(*name, name->block, accept);
    if (const auto* name = cursiv::ast_cast<ast::BlockAddressConst>(&n))
      check_name<&ast::BlockAddressConst::function>(*name, name->function,
                                                    accept);
    if (const auto* name = cursiv::ast_cast<ast::BlockAddressConst>(&n))
      check_name<&ast::BlockAddressConst::block>(*name, name->block, accept);
    if (const auto* name = cursiv::ast_cast<ast::Instruction>(&n))
      check_name<&ast::Instruction::result>(*name, name->result, accept);
    if (const auto* name = cursiv::ast_cast<ast::UseListOrderBB>(&n))
      check_name<&ast::UseListOrderBB::func>(*name, name->func, accept);
    if (const auto* name = cursiv::ast_cast<ast::UseListOrderBB>(&n))
      check_name<&ast::UseListOrderBB::block>(*name, name->block, accept);
    if (const auto* ref = cursiv::ast_cast<ast::MdRef>(&n)) {
      const auto id = unsigned_value(ref->id.substr(1));
      if (!id || !definitions.contains(*id))
        accept.error(*ref, "undefined metadata reference");
    }
    services.astReflection->visitMembers(
        n, [&](auto, auto, auto& child) { visit(child); });
  };
  visit(node);
}

void register_validation_checks(LlvmIrCoreServices& services) {
  using V = LlvmIrValidator;
  services.validator = std::make_unique<const V>(services);
  const auto& validator = *services.validator;
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<&V::checkType>(
           validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkAttribute>(
           validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkInstruction>(
           validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkMetadata>(
           validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkModule>(
           validator)});
}

}  // namespace llvmir
