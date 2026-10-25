#pragma once
#include "blueprint/core/ast.hpp"
#include "cursiv/core/services/JsonValue.hpp"
#ifndef AUDIT_BEFORE
#include "blueprint/core/Value.hpp"
#endif
#include <type_traits>
using J=cursiv::JsonValue;
namespace audit {
namespace a=blueprint::ast;
inline std::string hex(std::string_view s){constexpr char ds[]="0123456789abcdef";std::string o;for(unsigned char c:s){o+=ds[c>>4];o+=ds[c&15];}return o;}
inline J expr(const a::Expr*);
inline J property(const a::Property&p){return J::Object{{"name",p.name},{"value",expr(p.value)}};}
template<class C>J properties(const C&v){J::Array o;for(auto*p:v)o.push_back(property(*p));return o;}
inline J node(const a::Variable&p){return J::Object{{"variable",p.name}};}
inline J node(const a::AnyPattern&p){return J::Object{{"any",p.binding}};}
inline J node(const a::DefaultPattern&){return J::Object{{"default",true}};}
inline J node(const a::ListValue&p){J::Array o;for(auto*x:p.items)o.push_back(expr(x));return J::Object{{"list",o}};}
inline J node(const a::MapValue&p){return J::Object{{"map",properties(p.properties)}};}
#ifndef AUDIT_BEFORE
inline J node(const a::IntegerLit&p){auto v=blueprint::integer_value(p);return v?J(J::Object{{"integer",std::to_string(*v)}}):J(J::Object{{"integer_raw",p.raw}});}
inline J node(const a::StringLit&p){return J::Object{{"string",hex(blueprint::string_value(p).text)}};}
#endif
inline J node(const a::Select&);
template<class V>J value(const V&v){return std::visit([](auto x)->J{using T=decltype(x);if constexpr(std::is_same_v<T,bool>)return x;else if constexpr(std::is_same_v<T,std::int64_t>)return J::Object{{"integer",std::to_string(x)}};else if constexpr(std::is_same_v<T,std::string_view>)return J::Object{{"string",hex(x)}};else return x?node(*x):J(nullptr);},v);}
inline J expr(const a::Expr*p){if(!p)return nullptr;J::Array o;for(auto&v:p->terms)o.push_back(value(v));return J::Object{{"terms",o}};}
inline J node(const a::Select&p){J::Array cs,bs;for(auto*c:p.conditions){J::Array as;for(auto&x:c->args){
#ifdef AUDIT_BEFORE
as.emplace_back(hex(x));
#else
as.emplace_back(hex(blueprint::string_value(*x).text));
#endif
}cs.emplace_back(J::Object{{"function",c->function},{"args",as}});}for(auto*b:p.cases){J::Array ps;for(auto&x:b->patterns)ps.push_back(value(x));bs.emplace_back(J::Object{{"patterns",ps},{"value",expr(b->value)}});}return J::Object{{"select",J::Object{{"conditions",cs},{"cases",bs}}}};}
inline J view(const a::Document&d){J::Array o;for(auto&v:d.definitions)std::visit([&](auto*p){using T=std::remove_cvref_t<decltype(*p)>;if constexpr(std::is_same_v<T,a::Assignment>)o.emplace_back(J::Object{{"assignment",p->name},{"append",p->append},{"value",expr(p->value)}});else o.emplace_back(J::Object{{"module",p->type},{"properties",properties(p->properties)}});},v);return o;}
}
