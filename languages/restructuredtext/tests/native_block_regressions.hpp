#pragma once

#include <cstdint>
#include <cstdio>
#include <string>
#include <string_view>
#include <unordered_set>
#include <variant>

namespace rst_blocks {

namespace ast = restructuredtext::ast;

template <class T, class Variant>
const T* node(const Variant& value) {
  const auto* found = std::get_if<T*>(&value);
  return found ? *found : nullptr;
}

template <class Services, class Check>
bool inspect(const Services& services, const char* id, const char* text,
             Check check) {
  const std::string source(text);
  const auto parsed = services.parser->parse(source);
  const auto* doc = parsed.template value_as<ast::Document>();
  const bool clean = parsed.accepted && !parsed.cst().recovered() &&
                     parsed.diagnostics.empty() && doc;
  if (!clean || parsed.cst().text() != source) {
    std::fprintf(stderr,
                 "rst blocks: %s: accepted=%d recovered=%d diagnostics=%zu\n%s\n",
                 id, int(parsed.accepted), int(parsed.cst().recovered()),
                 parsed.diagnostics.size(), text);
    return false;
  }
  bool owned = !doc->parent;
  std::unordered_set<const cursiv::AstNode*> seen;
  const auto walk = [&](auto&& self, const cursiv::AstNode& current) -> void {
    if (!seen.insert(&current).second || current.cstNodeId >= parsed.cst().size()) {
      owned = false;
      return;
    }
    const auto& span = parsed.cst()[current.cstNodeId];
    owned &= span.begin <= span.end && span.end <= source.size();
    services.astReflection->visitMembers(
        current, [&](std::string_view, std::uint32_t, cursiv::AstNode& child) {
          owned &= child.parent == &current &&
                   parsed.cst().contains(current.cstNodeId, child.cstNodeId);
          self(self, child);
        });
  };
  walk(walk, *doc);
  if (owned && seen.size() == parsed.nodes.size() && check(*doc)) return true;
  std::fprintf(stderr, "rst blocks: %s: wrong AST or ownership\n%s\n", id, text);
  return false;
}

template <class Services>
bool run(const Services& services) {
  struct Doctest {
    const char* id;
    const char* source;
    std::string_view text;
  };
  for (const auto& c : {
           Doctest{"doctest-output", ">>> print(1)\n1\n", ">>> print(1)\n1"},
           Doctest{"doctest-raw-markup", ">>> '*raw*'\n**also raw**\n",
                   ">>> '*raw*'\n**also raw**"},
           Doctest{"doctest-prompt-only", ">>>", ">>>"},
           Doctest{"doctest-crlf", ">>> 1\r\n1\r\n", ">>> 1\r\n1"}}) {
    if (!inspect(services, c.id, c.source, [&](const ast::Document& doc) {
          const auto* block = doc.blocks.size() == 1
              ? node<ast::DoctestBlock>(doc.blocks[0]) : nullptr;
          return block && block->text == c.text && block->parent == &doc;
        })) return false;
  }
  if (!inspect(services, "doctest-blank-boundary",
               ">>> 1\n1\n \t\nAfter.\n", [](const ast::Document& doc) {
        const auto* block = doc.blocks.size() == 2
            ? node<ast::DoctestBlock>(doc.blocks[0]) : nullptr;
        return block && block->text == ">>> 1\n1" &&
               node<ast::Paragraph>(doc.blocks[1]);
      })) return false;
  if (!inspect(services, "doctest-two-blocks", ">>> one\none\n\n>>> two\ntwo",
               [](const ast::Document& doc) {
        if (doc.blocks.size() != 2) return false;
        const auto* first = node<ast::DoctestBlock>(doc.blocks[0]);
        const auto* second = node<ast::DoctestBlock>(doc.blocks[1]);
        return first && second && first->text == ">>> one\none" &&
               second->text == ">>> two\ntwo";
      })) return false;
  if (!inspect(services, "doctest-in-quote", "Before.\n\n  >>> 1\n  1\n",
               [](const ast::Document& doc) {
        const auto* quote = doc.blocks.size() == 2
            ? node<ast::BlockQuote>(doc.blocks[1]) : nullptr;
        const auto* block = quote && quote->body.size() == 1
            ? node<ast::DoctestBlock>(quote->body[0]) : nullptr;
        // text is a source capture, not Docutils' dedented rendering.
        return block && block->text == ">>> 1\n  1" && block->parent == quote;
      })) return false;

  if (!inspect(services, "field-pair", ":Date: 2001-08-16\n:Version: 1\n",
               [](const ast::Document& doc) {
        const auto* list = doc.blocks.size() == 1
            ? node<ast::FieldList>(doc.blocks[0]) : nullptr;
        return list && list->fields.size() == 2 &&
               list->fields[0]->name == "Date" &&
               list->fields[1]->name == "Version" &&
               list->fields[0]->body.size() == 1 &&
               node<ast::Paragraph>(list->fields[0]->body[0]) &&
               list->fields[1]->body.size() == 1 &&
               node<ast::Paragraph>(list->fields[1]->body[0]);
      })) return false;
  for (const char* source : {":Empty:", ":Empty:\n"}) {
    if (!inspect(services, "field-empty", source, [](const ast::Document& doc) {
          const auto* list = doc.blocks.size() == 1
              ? node<ast::FieldList>(doc.blocks[0]) : nullptr;
          return list && list->fields.size() == 1 &&
                 list->fields[0]->name == "Empty" &&
                 list->fields[0]->body.empty();
        })) return false;
  }
  struct FieldName {
    const char* id;
    const char* source;
    std::string_view name;
  };
  for (const auto& c : {
           FieldName{"field-colon", ":type:role: value\n", "type:role"},
           FieldName{"field-escaped-colon", ":name\\: arg: value\n", "name\\: arg"},
           FieldName{"field-markup-name", ":*Label*: value\n", "*Label*"},
           FieldName{"field-unicode-name", ":Données: valeur\n", "Données"}}) {
    if (!inspect(services, c.id, c.source, [&](const ast::Document& doc) {
          const auto* list = doc.blocks.size() == 1
              ? node<ast::FieldList>(doc.blocks[0]) : nullptr;
          return list && list->fields.size() == 1 &&
                 list->fields[0]->name == c.name &&
                 list->fields[0]->body.size() == 1;
        })) return false;
  }
  if (!inspect(services, "field-continuation-paragraphs",
               ":Name: *Ada*\n  and text\n\n  second paragraph\n\nTail.\n",
               [](const ast::Document& doc) {
        const auto* list = doc.blocks.size() == 2
            ? node<ast::FieldList>(doc.blocks[0]) : nullptr;
        const auto* field = list && list->fields.size() == 1
            ? list->fields[0] : nullptr;
        const auto* first = field && field->body.size() == 2
            ? node<ast::Paragraph>(field->body[0]) : nullptr;
        const auto* emphasis = first && !first->spans.empty()
            ? node<ast::Emphasis>(first->spans[0]) : nullptr;
        return field && field->name == "Name" && emphasis &&
               emphasis->text == "Ada" &&
               node<ast::Paragraph>(field->body[1]) &&
               node<ast::Paragraph>(doc.blocks[1]);
      })) return false;
  if (!inspect(services, "field-block-body",
               ":Items:\n\n  - first\n  - second\n", [](const ast::Document& doc) {
        const auto* list = doc.blocks.size() == 1
            ? node<ast::FieldList>(doc.blocks[0]) : nullptr;
        const auto* field = list && list->fields.size() == 1
            ? list->fields[0] : nullptr;
        return field && field->body.size() == 2 &&
               node<ast::BulletItem>(field->body[0]) &&
               node<ast::BulletItem>(field->body[1]);
      })) return false;
  if (!inspect(services, "field-nested", ":Outer:\n  :Inner: value\n",
               [](const ast::Document& doc) {
        const auto* list = doc.blocks.size() == 1
            ? node<ast::FieldList>(doc.blocks[0]) : nullptr;
        const auto* outer = list && list->fields.size() == 1
            ? list->fields[0] : nullptr;
        const auto* inner = outer && outer->body.size() == 1
            ? node<ast::FieldList>(outer->body[0]) : nullptr;
        return outer && outer->name == "Outer" && inner &&
               inner->fields.size() == 1 && inner->fields[0]->name == "Inner";
      })) return false;
  if (!inspect(services, "field-in-item", "- item\n\n  :Name: value\n",
               [](const ast::Document& doc) {
        const auto* item = doc.blocks.size() == 1
            ? node<ast::BulletItem>(doc.blocks[0]) : nullptr;
        const auto* list = item && item->body.size() == 2
            ? node<ast::FieldList>(item->body[1]) : nullptr;
        return list && list->fields.size() == 1 && list->fields[0]->name == "Name";
      })) return false;
  for (const auto& source : {"- item\n\n:Name: value\n",
                             "term\n  definition\n\n:Name: value\n"}) {
    if (!inspect(services, "field-after-other-block", source,
                 [](const ast::Document& doc) {
          return doc.blocks.size() == 2 &&
                 node<ast::FieldList>(doc.blocks[1]);
        })) return false;
  }
  if (!inspect(services, "field-after-title", "Title\n=====\n\n:Name: value\n",
               [](const ast::Document& doc) {
        if (doc.blocks.size() != 2) return false;
        const auto* section = node<ast::Section>(doc.blocks[0]);
        const auto* list = node<ast::FieldList>(doc.blocks[1]);
        const auto body = ast::section_body(doc, section);
        return section && list && body.size() == 1 &&
               node<ast::FieldList>(body[0]) == list;
      })) return false;
  if (!inspect(services, "field-role-body", ":emphasis: \x60value\x60\n",
               [](const ast::Document& doc) {
        const auto* list = doc.blocks.size() == 1
            ? node<ast::FieldList>(doc.blocks[0]) : nullptr;
        const auto* field = list && list->fields.size() == 1
            ? list->fields[0] : nullptr;
        const auto* paragraph = field && field->body.size() == 1
            ? node<ast::Paragraph>(field->body[0]) : nullptr;
        const auto* text = paragraph && paragraph->spans.size() == 1
            ? node<ast::InterpretedText>(paragraph->spans[0]) : nullptr;
        return field && field->name == "emphasis" && text &&
               text->role.empty() && text->text == "value";
      })) return false;

  // These inputs are valid ordinary paragraphs. They are negative controls
  // for recognizing either of the new block families, not invalid documents.
  for (const char* source : {">>>not a prompt\n", "x >>> prompt\n",
                             "ordinary\n>>> 1\n2\n",
                             "ordinary\n:Name: text\n", ":Name:text\n",
                             ": Name: text\n", ":Name : text\n", ":: text\n"}) {
    if (!inspect(services, "plain-text-fallback", source,
                 [](const ast::Document& doc) {
          return doc.blocks.size() == 1 &&
                 node<ast::Paragraph>(doc.blocks[0]);
        })) return false;
  }
  if (!inspect(services, "role-prefix-preserved", ":emphasis:\x60value\x60\n",
               [](const ast::Document& doc) {
        const auto* paragraph = doc.blocks.size() == 1
            ? node<ast::Paragraph>(doc.blocks[0]) : nullptr;
        const auto* text = paragraph && paragraph->spans.size() == 1
            ? node<ast::InterpretedText>(paragraph->spans[0]) : nullptr;
        return text && text->role == "emphasis" && text->text == "value";
      })) return false;
  return true;
}

}  // namespace rst_blocks
