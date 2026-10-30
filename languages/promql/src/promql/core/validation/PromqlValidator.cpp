#include "promql/core/validation/PromqlValidator.hpp"

#include <charconv>
#include <memory>
#include <string>
#include <cmath>
#include <optional>
#include <limits>
#include "promql/core/validation/Functions.hpp"
#include "cursiv/core/workspace/Document.hpp"

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "promql/core/CoreServices.hpp"

namespace promql {
namespace {

bool duration_expression(const ast::Expression* expression) {
  if (!expression) return true;
  if (cursiv::ast_cast<ast::NumberLit>(expression) ||
      cursiv::ast_cast<ast::DurationLit>(expression)) return true;
  if (const auto* unary = cursiv::ast_cast<ast::UnaryExpr>(expression))
    return duration_expression(unary->operand);
  if (const auto* binary = cursiv::ast_cast<ast::BinaryExpr>(expression)) {
    switch (binary->op) {
      case ast::Operator::add: case ast::Operator::subtract:
      case ast::Operator::multiply: case ast::Operator::divide:
      case ast::Operator::modulo: case ast::Operator::pow:
        return duration_expression(binary->left) && duration_expression(binary->right);
      default: return false;
    }
  }
  if (const auto* call = cursiv::ast_cast<ast::FunctionCall>(expression)) {
    auto name = std::string(call->name);
    for (auto& c : name)
      if (c >= 'A' && c <= 'Z') c += 'a' - 'A';
    if (name == "step" || name == "range") return call->args.empty();
    if (name == "min_of" || name == "max_of")
      return call->args.size() == 2 && duration_expression(call->args[0]) &&
             duration_expression(call->args[1]);
  }
  return false;
}

std::string lower(std::string_view text) {
  std::string value(text);
  for(auto& c:value)if(c>='A'&&c<='Z')c+= 'a'-'A';
  return value;
}
bool durationFunction(std::string_view name) {
  const auto text=lower(name);
  return text=="step"||text=="range"||text=="start"||text=="end"||text=="min_of"||text=="max_of";
}
bool durationContext(const cursiv::AstNode& node) {
  const cursiv::AstNode* child=&node;
  for(const auto* parent=node.parent;parent;child=parent,parent=parent->parent) {
    if(const auto* selector=cursiv::ast_cast<ast::Selector>(parent))return child==selector->range||child==selector->offset;
    if(const auto* sub=cursiv::ast_cast<ast::Subquery>(parent))return child==sub->range||child==sub->step||child==sub->offset;
    if(const auto* timestamp=cursiv::ast_cast<ast::Timestamped>(parent))return child==timestamp->offset;
  }
  return false;
}
char expressionType(const ast::Expression* expression) {
  if(!expression)return '?';
  if(cursiv::ast_cast<ast::NumberLit>(expression)||cursiv::ast_cast<ast::DurationLit>(expression))return 's';
  if(cursiv::ast_cast<ast::StringLit>(expression))return 't';
  if(const auto* selector=cursiv::ast_cast<ast::Selector>(expression))return selector->range?'m':'v';
  if(cursiv::ast_cast<ast::Subquery>(expression))return 'm';
  if(cursiv::ast_cast<ast::AggregationExpr>(expression))return 'v';
  if(const auto* call=cursiv::ast_cast<ast::FunctionCall>(expression)) {
    auto name=std::string(call->name);
    if(durationContext(*call)&&durationFunction(name))name=lower(name);
    if(const auto* signature=functionSignature(name))return signature->result;
  }
  if(const auto* unary=cursiv::ast_cast<ast::UnaryExpr>(expression))return expressionType(unary->operand);
  if(const auto* timestamp=cursiv::ast_cast<ast::Timestamped>(expression))return expressionType(timestamp->operand);
  if(const auto* modifier=cursiv::ast_cast<ast::RangeModifier>(expression))return expressionType(modifier->operand);
  if(const auto* binary=cursiv::ast_cast<ast::BinaryExpr>(expression))
    return expressionType(binary->left)=='v'||expressionType(binary->right)=='v'?'v':'s';
  return '?';
}
std::optional<double> durationValue(std::string_view text) {
  unsigned last=8; long double total=0;
  for(std::size_t i=0;i<text.size();) {
    long double value=0;
    while(i<text.size()&&text[i]>='0'&&text[i]<='9')value=value*10+(text[i++]-'0');
    const bool ms=text.substr(i,2)=="ms";
    const char unit=i<text.size()?text[i]:'?';
    const unsigned rank=ms?1:unit=='s'?2:unit=='m'?3:unit=='h'?4:unit=='d'?5:unit=='w'?6:unit=='y'?7:0;
    if(rank==0||rank>=last)return {};
    last=rank;i+=ms?2:1;
    const long double factor=ms?.001L:unit=='s'?1:unit=='m'?60:unit=='h'?3600:unit=='d'?86400:unit=='w'?604800:31536000;
    total+=value*factor;
    if(total>static_cast<long double>(INT64_MAX)/1000000000.L)return {};
  }
  return static_cast<double>(total);
}
std::optional<double> constant(const ast::Expression* expression) {
  if(const auto* number=cursiv::ast_cast<ast::NumberLit>(expression))return number->value;
  if(const auto* duration=cursiv::ast_cast<ast::DurationLit>(expression))return durationValue(duration->text);
  if(const auto* unary=cursiv::ast_cast<ast::UnaryExpr>(expression)) {
    if(auto value=constant(unary->operand))return unary->op==ast::Operator::subtract?-*value:*value;
  }
  return {};
}
bool hasOffset(const ast::Expression* expression) {
  if(const auto* selector=cursiv::ast_cast<ast::Selector>(expression))return selector->offset;
  if(const auto* sub=cursiv::ast_cast<ast::Subquery>(expression))return sub->offset;
  if(const auto* timestamp=cursiv::ast_cast<ast::Timestamped>(expression))return timestamp->offset||hasOffset(timestamp->operand);
  if(const auto* modifier=cursiv::ast_cast<ast::RangeModifier>(expression))return hasOffset(modifier->operand);
  return false;
}
void positiveRange(const ast::Expression* expression,cursiv::ValidationAcceptor& accept) {
  if(const auto value=constant(expression);value&&(!std::isfinite(*value)||*value<=0))
    accept.error(*expression,"A literal range or resolution must be finite and greater than zero.","promql.range");
}
}  // namespace

void PromqlValidator::checkSelectorDurations(
    const ast::Selector& node, cursiv::ValidationAcceptor& accept) const {
  positiveRange(node.range,accept);
  if (!duration_expression(node.range))
    accept.error(node, "A range must be a duration expression.")
        .property<&ast::Selector::range>();
  if (!duration_expression(node.offset))
    accept.error(node, "An offset must be a duration expression.")
        .property<&ast::Selector::offset>();
}

void PromqlValidator::checkSubqueryDurations(
    const ast::Subquery& node, cursiv::ValidationAcceptor& accept) const {
  positiveRange(node.range,accept);
  if (!duration_expression(node.range))
    accept.error(node, "A range must be a duration expression.")
        .property<&ast::Subquery::range>();
  positiveRange(node.step,accept);
  if (!duration_expression(node.step))
    accept.error(node, "A step must be a duration expression.")
        .property<&ast::Subquery::step>();
  if (!duration_expression(node.offset))
    accept.error(node, "An offset must be a duration expression.")
        .property<&ast::Subquery::offset>();
}

void PromqlValidator::checkTimestampTarget(
    const ast::Timestamped& node, cursiv::ValidationAcceptor& accept) const {
  const ast::Expression* operand = node.operand;
  while (const auto* modified = cursiv::ast_cast<ast::RangeModifier>(operand))
    operand = modified->operand;
  if (operand && operand->type != &cursiv::ast_type_tag<ast::Selector> &&
      operand->type != &cursiv::ast_type_tag<ast::Subquery>)
    accept.error(node,
                 "The @ modifier applies to a vector selector or subquery.");
  const auto* unary = cursiv::ast_cast<ast::UnaryExpr>(node.timestamp);
  if (node.timestamp &&
      node.timestamp->type != &cursiv::ast_type_tag<ast::NumberLit> &&
      node.timestamp->type != &cursiv::ast_type_tag<ast::DurationLit> &&
      node.timestamp->type != &cursiv::ast_type_tag<ast::TimestampBoundary> &&
      !(unary && (cursiv::ast_cast<ast::NumberLit>(unary->operand) ||
                  cursiv::ast_cast<ast::DurationLit>(unary->operand))))
    accept.error(node, "The @ timestamp must be a number, start() or end().")
        .property<&ast::Timestamped::timestamp>();
  if (!duration_expression(node.offset))
    accept.error(node, "An offset must be a duration expression.")
        .property<&ast::Timestamped::offset>();
  if(node.offset&&hasOffset(node.operand))accept.error(node,"Offset may not be set twice.","promql.offset");
  if(const auto value=constant(node.timestamp);value&&!std::isfinite(*value))
    accept.error(node,"Timestamp must be finite.","promql.timestamp").property<&ast::Timestamped::timestamp>();
}

void PromqlValidator::checkDuration(const ast::DurationLit& node,cursiv::ValidationAcceptor& accept) const {
  if(!durationValue(node.text))accept.error(node,"Duration units must be ordered, unique and within range.","promql.duration")
      .property<&ast::DurationLit::text>();
}
void PromqlValidator::checkFunction(const ast::FunctionCall& node,cursiv::ValidationAcceptor& accept) const {
  auto name=std::string(node.name);
  if(durationContext(node)&&durationFunction(name))name=lower(name);
  const auto* signature=functionSignature(name);
  if(!signature) { accept.error(node,"Unknown PromQL function.","promql.function").property<&ast::FunctionCall::name>();return; }
  const auto count=signature->args.size();
  const auto minimum=signature->variadic?count-1:count;
  if(node.args.size()<minimum||(!signature->variadic&&node.args.size()!=count)||
      (signature->variadic>0&&node.args.size()>minimum+signature->variadic)) {
    accept.error(node,"Wrong number of function arguments.","promql.function-arity");return;
  }
  for(std::size_t i=0;i<node.args.size();++i) {
    const auto expected=signature->args[i<count?i:count-1];
    const auto actual=expressionType(node.args[i]);
    if(actual!='?'&&actual!=expected)accept.error(*node.args[i],"Function argument has the wrong type.","promql.function-type");
  }
}
void PromqlValidator::checkAggregation(const ast::AggregationExpr& node,cursiv::ValidationAcceptor& accept) const {
  const auto name=lower(node.op);
  const bool parameter=name=="topk"||name=="bottomk"||name=="quantile"||name=="limitk"||name=="limit_ratio"||name=="count_values";
  const std::size_t expected=parameter?2:1;
  if(node.args.size()!=expected) { accept.error(node,"Wrong number of aggregation arguments.","promql.aggregation");return; }
  if(expressionType(node.args.back())!='v')accept.error(*node.args.back(),"Aggregation requires an instant vector.","promql.aggregation");
  if(parameter&&expressionType(node.args[0])!=(name=="count_values"?'t':'s'))
    accept.error(*node.args[0],"Aggregation parameter has the wrong type.","promql.aggregation");
}
void PromqlValidator::checkBinary(const ast::BinaryExpr& node,cursiv::ValidationAcceptor& accept) const {
  using O=ast::Operator;
  const bool comparison=node.op==O::deq||node.op==O::ne||node.op==O::gt||node.op==O::lt||node.op==O::ge||node.op==O::le;
  const bool set=node.op==O::logical_and||node.op==O::logical_or||node.op==O::logical_unless;
  const char left=expressionType(node.left),right=expressionType(node.right);
  if(node.boolMod&&!comparison)accept.error(node,"BOOL applies only to comparisons.","promql.binary");
  if((left!='?'&&left!='s'&&left!='v')||(right!='?'&&right!='s'&&right!='v'))
    accept.error(node,"Binary operands must be scalars or instant vectors.","promql.binary");
  if(comparison&&left=='s'&&right=='s'&&!node.boolMod)
    accept.error(node,"Scalar comparison requires BOOL.","promql.binary");
  if(set&&(left!='v'||right!='v'))accept.error(node,"Set operators require two instant vectors.","promql.binary");
  if(node.matching&&(left!='v'||right!='v'))accept.error(node,"Vector matching requires two instant vectors.","promql.binary");
  if(set&&node.matching&&node.matching->hasSide)accept.error(node,"Set operators do not permit grouping sides.","promql.binary");
  if(node.matching&&!node.matching->ignoring)
    for(const auto name:node.matching->labels)for(const auto included:node.matching->sideLabels)
      if(name==included)accept.error(node,"A label cannot occur in ON and GROUP at once.","promql.binary");
  if(node.fills.size()>2 || (node.fills.size()==2&&
      (lower(node.fills[0]->name)=="fill"||lower(node.fills[1]->name)=="fill"||
       lower(node.fills[0]->name)==lower(node.fills[1]->name))))
    accept.error(node,"Use fill alone, or one fill_left and one fill_right.","promql.binary");
  if(!node.fills.empty()&&(set||left!='v'||right!='v'))
    accept.error(node,"Fill modifiers require two instant vectors and a non-set operator.","promql.binary");
  if(durationContext(node)&&(node.op==O::divide||node.op==O::modulo))
    if(const auto value=constant(node.right);value&&*value==0)accept.error(node,"Division by zero in duration expression.","promql.duration");
}
void PromqlValidator::checkUnary(const ast::UnaryExpr& node,cursiv::ValidationAcceptor& accept) const {
  const char type=expressionType(node.operand);
  if(type!='?'&&type!='s'&&type!='v')accept.error(node,"Unary operands must be scalars or instant vectors.","promql.unary");
}
void PromqlValidator::checkSubqueryType(const ast::Subquery& node,cursiv::ValidationAcceptor& accept) const {
  if(expressionType(node.operand)!='v')accept.error(node,"A subquery requires an instant vector.","promql.subquery");
}
void PromqlValidator::checkSelector(const ast::Selector& node,cursiv::ValidationAcceptor& accept) const {
  bool canDecide=true, matchesEmpty=true;
  for(const auto* matcher:node.matchers) {
    if(!node.metric.empty()&&matcher->name=="__name__")
      accept.error(*matcher,"Metric name must not be set twice.","promql.selector");
    if(matcher->op==ast::Operator::eq)matchesEmpty &= matcher->value.empty();
    else if(matcher->op==ast::Operator::ne)matchesEmpty &= !matcher->value.empty();
    else if(matcher->value.empty()||matcher->value==".*"||matcher->value=="^.*$"||matcher->value=="^$")
      matchesEmpty &= matcher->op==ast::Operator::re;
    else canDecide=false;
    if(matcher->name.find(':')!=std::string_view::npos) {
      const auto* document=matcher->getDocument();
      const auto span=accept.featureSpan(*matcher,"name",0);
      if(document&&span&&span->begin<document->textDocument().getText().size()) {
        const char first=document->textDocument().getText()[span->begin];
        if(first!='\''&&first!='"'&&first!='`')
          accept.error(*matcher,"A colon-containing label name must be quoted.","promql.label");
      }
    }
  }
  if(node.metric.empty()&&canDecide&&matchesEmpty)
    accept.error(node,"A vector selector needs a nonempty matcher.","promql.selector");
}
void PromqlValidator::checkRangeModifier(const ast::RangeModifier& node,cursiv::ValidationAcceptor& accept) const {
  const ast::Expression* expression=node.operand;
  while(expression) {
    if(const auto* inner=cursiv::ast_cast<ast::RangeModifier>(expression)) {
      if(lower(inner->name)!=lower(node.name))accept.error(node,"Anchored and smoothed cannot be combined.","promql.range-modifier");
      expression=inner->operand;
    } else if(const auto* timestamp=cursiv::ast_cast<ast::Timestamped>(expression))expression=timestamp->operand;
    else break;
  }
  if(!cursiv::ast_cast<ast::Selector>(expression))
    accept.error(node,"Range modifier requires a vector selector.","promql.range-modifier");
}
void register_validation_checks(PromqlCoreServices& services) {
  using V = PromqlValidator;
  services.validator = std::make_unique<const V>(services);
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<&V::checkDuration>(*services.validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkFunction>(*services.validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkAggregation>(*services.validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkBinary>(*services.validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkUnary>(*services.validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkSubqueryType>(*services.validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkSelector>(*services.validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkRangeModifier>(*services.validator),
       cursiv::ValidationRegistry::makeValidationCheck<
          &V::checkSelectorDurations>(*services.validator),
       cursiv::ValidationRegistry::makeValidationCheck<
          &V::checkSubqueryDurations>(*services.validator),
       cursiv::ValidationRegistry::makeValidationCheck<
          &PromqlValidator::checkTimestampTarget>(*services.validator)});
}

}  // namespace promql
