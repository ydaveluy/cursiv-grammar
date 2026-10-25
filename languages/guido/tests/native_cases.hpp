#pragma once
#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include "cursiv/test/ValidationResult.hpp"
#include "native_projection.hpp"
#include <fstream>
#include <unordered_set>
namespace native_regressions {
inline int run(const auto &language) {
  int failures = 0;
  struct Case {
    const char *file;
    const char *expected;
    const char *code;
  };
  const Case
      cases
          [] =
              {
                  {"00.gmn",
                   R"GMN([["score"],["voice"],["note","c"],["append"],["note","d"],["append"],["note","e"],["append"],["end-voice"],["end-score"]])GMN",
                   ""},
                  {"01.gmn",
                   R"GMN([["score"],["voice"],["end-voice"],["end-score"]])GMN",
                   ""},
                  {"02.gmn", R"GMN([["score"],["end-score"]])GMN", ""},
                  {"03.gmn",
                   R"GMN([["score"],["voice"],["end-voice"],["append-voice"],["voice"],["note","c"],["append"],["end-voice"],["append-voice"],["end-score"]])GMN",
                   ""},
                  {"04.gmn",
                   R"GMN([["score"],["voice"],["note","c"],["append"],["note","d"],["append"],["end-voice"],["end-score"]])GMN",
                   ""},
                  {"05.gmn",
                   R"GMN([["score"],["voice"],["note","do"],["append"],["note","re"],["append"],["note","mi"],["append"],["note","fa"],["append"],["note","sol"],["append"],["note","la"],["append"],["note","si"],["append"],["note","ti"],["append"],["end-voice"],["end-score"]])GMN",
                   ""},
                  {"06.gmn",
                   R"GMN([["score"],["voice"],["note","cis"],["append"],["note","dis"],["append"],["note","fis"],["append"],["note","gis"],["append"],["note","ais"],["append"],["end-voice"],["end-score"]])GMN",
                   ""},
                  {"07.gmn",
                   R"GMN([["score"],["voice"],["note","c"],["acc",1],["acc",1],["acc",-1],["oct",-1],["enum",3],["denom",8],["dots",3],["append"],["end-voice"],["end-score"]])GMN",
                   ""},
                  {"08.gmn",
                   R"GMN([["score"],["voice"],["note","c"],["ms",100],["append"],["end-voice"],["end-score"]])GMN",
                   ""},
                  {"09.gmn",
                   R"GMN([["score"],["voice"],["tag","\\bar",0],["add-tag"],["end-tag"],["end-voice"],["end-score"]])GMN",
                   ""},
                  {"10.gmn",
                   R"GMN([["score"],["voice"],["tag","\\accent",0],["add-tag"],["range"],["note","c"],["append"],["end-tag"],["end-voice"],["end-score"]])GMN",
                   ""},
                  {"11.gmn",
                   R"GMN([["score"],["voice"],["tag","\\dynam",0],["parameter","string","","f",""],["add-tag"],["end-tag"],["end-voice"],["end-score"]])GMN",
                   ""},
                  {"12.gmn",
                   R"GMN([["score"],["voice"],["tag","\\my_tag2",0],["parameter","integer","","3",""],["add-tag"],["end-tag"],["end-voice"],["end-score"]])GMN",
                   ""},
                  {"13.gmn",
                   R"GMN([["score"],["voice"],["tag","\\slur",12],["add-tag"],["range"],["note","c"],["append"],["note","d"],["append"],["end-tag"],["end-voice"],["end-score"]])GMN",
                   ""},
                  {"14.gmn",
                   R"GMN([["score"],["voice"],["tag","\\text",0],["parameter","integer","dx","2","hs"],["parameter","integer","dy","-3","cm"],["add-tag"],["range"],["note","c"],["append"],["end-tag"],["end-voice"],["end-score"]])GMN",
                   ""},
                  {"15.gmn",
                   R"GMN([["score"],["voice"],["tag","\\tempo",0],["parameter","real","","0.5",""],["parameter","real","","1.25",""],["parameter","real","","-0.25",""],["add-tag"],["end-tag"],["end-voice"],["end-score"]])GMN",
                   ""},
                  {"16.gmn",
                   R"GMN([["score"],["voice"],["tag","\\title",0],["parameter","string","","hello",""],["add-tag"],["end-tag"],["end-voice"],["end-score"]])GMN",
                   ""},
                  {"17.gmn",
                   R"GMN([["score"],["voice"],["tag","\\title",0],["parameter","string","","A\"B\\C\\nD",""],["add-tag"],["end-tag"],["end-voice"],["end-score"]])GMN",
                   ""},
                  {"18.gmn",
                   R"GMN([["score"],["voice"],["tag","\\slur",0],["add-tag"],["range"],["note","c"],["append"],["tag","\\accent",0],["add-tag"],["range"],["note","d"],["append"],["end-tag"],["note","e"],["append"],["end-tag"],["end-voice"],["end-score"]])GMN",
                   ""},
                  {"19.gmn",
                   R"GMN([["score"],["voice"],["tag","\\slur",0],["add-tag"],["range"],["end-tag"],["end-voice"],["end-score"]])GMN",
                   ""},
                  {"20.gmn",
                   R"GMN([["score"],["voice"],["chord"],["member"],["tag","\\accent",0],["add-tag"],["end-tag"],["note","c"],["append"],["member"],["note","d"],["append"],["tag","\\accent",0],["add-tag"],["end-tag"],["append-chord"],["end-voice"],["end-score"]])GMN",
                   ""},
                  {"21.gmn",
                   R"GMN([["score"],["voice"],["chord"],["member"],["tag","\\slur",0],["add-tag"],["range"],["note","c"],["append"],["end-tag"],["member"],["note","e"],["append"],["append-chord"],["end-voice"],["end-score"]])GMN", ""},
                  {"22.gmn",
                   R"GMN([["score"],["voice"],["note","c"],["denom",4],["append"],["end-voice"],["end-score"]])GMN",
                   ""},
                  {"23.gmn",
                   R"GMN([["score"],["voice"],["note","_"],["denom",8],["append"],["end-voice"],["end-score"]])GMN",
                   ""},
                  {"24.gmn",
                   R"GMN([["score"],["voice"],["note","empty"],["enum",2],["append"],["end-voice"],["end-score"]])GMN",
                   ""},
                  {"25.gmn",
                   R"GMN([["score"],["voice"],["note","c"],["append"],["end-voice"],["end-score"]])GMN",
                   ""},
                  {"26.gmn",
                   R"GMN([["score"],["voice"],["note","c"],["append"],["note","d"],["append"],["end-voice"],["end-score"]])GMN",
                   ""},
                  {"27.gmn",
                   R"GMN([["variable-declaration","$music","c d",0],["score"],["voice"],["variable-symbols","$music"],["end-voice"],["end-score"]])GMN",
                   ""},
                  {"28.gmn",
                   R"GMN([["variable-declaration","$n","2",1],["score"],["voice"],["tag","\\staff",0],["parameter","variable","","$n",""],["add-tag"],["end-tag"],["note","c"],["append"],["end-voice"],["end-score"]])GMN",
                   ""},
                  {"29.gmn",
                   R"GMN([["variable-declaration","$n",".5",2],["score"],["voice"],["tag","\\tempo",0],["parameter","variable","value","$n",""],["add-tag"],["end-tag"],["note","c"],["append"],["end-voice"],["end-score"]])GMN",
                   ""},
                  {"30.gmn",
                   R"GMN([["score"],["voice"],["tag","\\tag",0],["add-tag"],["end-tag"],["end-voice"],["end-score"]])GMN",
                   ""},
                  {"31.gmn",
                   R"GMN([["score"],["voice"],["tag","\\tag",0],["add-tag"],["end-tag"],["end-voice"],["end-score"]])GMN",
                   ""},
                  {"32.gmn",
                   R"GMN([["score"],["voice"],["note","c"],["oct",2],["denom",16],["dots",2],["append"],["end-voice"],["end-score"]])GMN",
                   ""},
                  {"33.gmn",
                   R"GMN([["score"],["voice"],["note","c"],["append"],["note","do"],["append"],["end-voice"],["end-score"]])GMN",
                   ""},
                  {"34.gmn",
                   R"GMN([["score"],["voice"],["note","h"],["append"],["end-voice"],["end-score"]])GMN",
                   ""},
                  {"37.gmn",
                   R"GMN([["score"],["voice"],["note","c"],["denom",0],["append"],["end-voice"],["end-score"]])GMN",
                   "guido.duration-denominator"},
                  {"42.gmn",
                   R"GMN([["score"],["voice"],["tag","\\clef",0],["parameter","string","","treble",""],["add-tag"],["end-tag"],["tag","\\key",0],["parameter","string","","D",""],["add-tag"],["end-tag"],["tag","\\meter",0],["parameter","string","","4/4",""],["add-tag"],["end-tag"],["note","a"],["oct",1],["enum",1],["denom",2],["append"],["note","b"],["append"],["note","a"],["denom",4],["dots",1],["append"],["note","g"],["denom",8],["append"],["note","f"],["acc",1],["denom",4],["append"],["note","g"],["append"],["note","a"],["denom",2],["append"],["note","b"],["append"],["note","a"],["denom",4],["dots",1],["append"],["note","g"],["denom",8],["append"],["note","f"],["acc",1],["denom",4],["append"],["note","g"],["append"],["note","a"],["denom",2],["append"],["note","a"],["append"],["note","b"],["append"],["note","c"],["acc",1],["oct",2],["denom",4],["append"],["note","d"],["append"],["note","c"],["acc",1],["denom",2],["append"],["note","b"],["oct",1],["append"],["note","a"],["denom",1],["append"],["end-voice"],["end-score"]])GMN", ""},
                  {"43.gmn",
                   R"GMN([["score"],["voice"],["tag","\\meter",0],["parameter","string","","4/4",""],["add-tag"],["end-tag"],["note","c"],["append"],["note","d"],["append"],["note","e"],["append"],["note","c"],["append"],["note","c"],["append"],["note","d"],["append"],["note","e"],["append"],["note","c"],["append"],["note","e"],["append"],["note","f"],["append"],["note","g"],["denom",2],["append"],["note","e"],["denom",4],["append"],["note","f"],["append"],["note","g"],["denom",2],["append"],["note","g"],["denom",8],["append"],["note","a"],["append"],["note","g"],["append"],["note","f"],["append"],["note","e"],["denom",4],["append"],["note","c"],["append"],["note","g"],["denom",8],["append"],["note","a"],["append"],["note","g"],["append"],["note","f"],["append"],["note","e"],["denom",4],["append"],["note","c"],["append"],["end-voice"],["append-voice"],["voice"],["tag","\\meter",0],["parameter","string","","4/4",""],["add-tag"],["end-tag"],["note","_"],["enum",8],["denom",4],["append"],["note","c"],["denom",4],["append"],["note","d"],["append"],["note","e"],["append"],["note","c"],["append"],["note","c"],["append"],["note","d"],["append"],["note","e"],["append"],["note","c"],["append"],["note","e"],["append"],["note","f"],["append"],["note","g"],["denom",2],["append"],["note","e"],["denom",4],["append"],["note","f"],["append"],["note","g"],["denom",2],["append"],["end-voice"],["append-voice"],["end-score"]])GMN",
                   ""},
                  {"44.gmn",
                   R"GMN([["score"],["voice"],["tag","\\barFormat",0],["parameter","string","","system",""],["add-tag"],["end-tag"],["tag","\\title",0],["parameter","string","","Leaf",""],["add-tag"],["end-tag"],["tag","\\staff",0],["parameter","integer","","1",""],["add-tag"],["end-tag"],["tag","\\clef",0],["parameter","string","","g2",""],["add-tag"],["end-tag"],["tag","\\key",0],["parameter","integer","","-2",""],["add-tag"],["end-tag"],["tag","\\meter",0],["parameter","string","","4/4",""],["add-tag"],["end-tag"],["tag","\\t",0],["parameter","string","","Cm7",""],["parameter","integer","","13",""],["add-tag"],["range"],["chord"],["member"],["note","e"],["acc",-1],["oct",1],["enum",1],["append"],["member"],["note","g"],["oct",1],["append"],["append-chord"],["end-tag"],["tag","\\t",0],["parameter","string","","F7",""],["parameter","integer","","13",""],["add-tag"],["range"],["chord"],["member"],["note","d"],["oct",1],["enum",1],["append"],["member"],["note","f"],["oct",1],["append"],["append-chord"],["end-tag"],["tag","\\t",0],["parameter","string","","BbM7",""],["parameter","integer","","13",""],["add-tag"],["range"],["chord"],["member"],["note","d"],["oct",1],["enum",1],["append"],["member"],["note","f"],["oct",1],["append"],["member"],["note","a"],["oct",1],["append"],["append-chord"],["end-tag"],["tag","\\t",0],["parameter","string","","EbM7",""],["parameter","integer","","13",""],["add-tag"],["range"],["chord"],["member"],["note","f"],["oct",1],["enum",1],["append"],["member"],["note","b"],["acc",-1],["oct",1],["append"],["member"],["note","d"],["oct",2],["append"],["append-chord"],["end-tag"],["tag","\\t",0],["parameter","string","","Am7b5",""],["parameter","integer","","13",""],["add-tag"],["range"],["chord"],["member"],["note","e"],["acc",-1],["oct",1],["enum",1],["append"],["member"],["note","g"],["oct",1],["append"],["member"],["note","c"],["oct",2],["append"],["append-chord"],["end-tag"],["tag","\\t",0],["parameter","string","","D7#9",""],["parameter","integer","","13",""],["parameter","integer","dx","2","hs"],["add-tag"],["range"],["chord"],["member"],["note","f"],["oct",1],["denom",2],["append"],["member"],["note","b"],["acc",-1],["oct",1],["append"],["member"],["note","d"],["oct",2],["append"],["append-chord"],["chord"],["member"],["note","e"],["acc",-1],["oct",1],["denom",2],["append"],["member"],["note","a"],["acc",-1],["oct",1],["append"],["member"],["note","c"],["oct",2],["append"],["append-chord"],["end-tag"],["tag","\\t",0],["parameter","string","","Gm7",""],["parameter","integer","","13",""],["add-tag"],["range"],["chord"],["member"],["note","f"],["oct",1],["enum",1],["append"],["member"],["note","b"],["acc",-1],["oct",1],["append"],["append-chord"],["end-tag"],["tag","\\t",0],["parameter","string","","G7#9",""],["parameter","integer","","16",""],["add-tag"],["range"],["chord"],["member"],["note","e"],["acc",-1],["oct",1],["denom",2],["append"],["member"],["note","g"],["oct",1],["append"],["member"],["note","b"],["acc",-1],["oct",1],["append"],["append-chord"],["chord"],["member"],["note","e"],["oct",1],["denom",2],["append"],["member"],["note","g"],["acc",1],["oct",1],["append"],["member"],["note","b"],["oct",1],["append"],["append-chord"],["end-tag"],["tag","\\repeatEnd",0],["add-tag"],["end-tag"],["tag","\\t",0],["parameter","string","","D7#9",""],["parameter","integer","","16",""],["add-tag"],["range"],["chord"],["member"],["note","e"],["acc",-1],["oct",1],["denom",2],["append"],["member"],["note","a"],["acc",-1],["oct",1],["append"],["member"],["note","c"],["oct",2],["append"],["append-chord"],["chord"],["member"],["note","f"],["oct",1],["denom",2],["append"],["member"],["note","b"],["acc",-1],["oct",1],["append"],["member"],["note","d"],["oct",2],["append"],["append-chord"],["end-tag"],["tag","\\t",0],["parameter","string","","Gm7",""],["parameter","integer","","13",""],["parameter","integer","dx","2","hs"],["add-tag"],["range"],["chord"],["member"],["note","f"],["oct",1],["denom",2],["append"],["member"],["note","b"],["acc",-1],["oct",1],["append"],["member"],["note","d"],["oct",2],["append"],["append-chord"],["chord"],["member"],["note","g"],["oct",1],["denom",2],["append"],["member"],["note","c"],["oct",2],["append"],["member"],["note","f"],["oct",2],["append"],["append-chord"],["end-tag"],["tag","\\t",0],["parameter","string","","Gm6",""],["parameter","integer","","13",""],["add-tag"],["range"],["chord"],["member"],["note","a"],["oct",1],["denom",2],["append"],["member"],["note","d"],["oct",2],["append"],["member"],["note","g"],["oct",2],["append"],["append-chord"],["chord"],["member"],["note","b"],["acc",-1],["oct",1],["denom",2],["append"],["member"],["note","d"],["oct",2],["append"],["member"],["note","g"],["oct",2],["append"],["append-chord"],["end-tag"],["tag","\\t",0],["parameter","string","","Am7b5",""],["parameter","integer","","13",""],["parameter","integer","dx","2","hs"],["add-tag"],["range"],["chord"],["member"],["note","e"],["acc",-1],["oct",1],["denom",2],["append"],["member"],["note","g"],["oct",1],["append"],["member"],["note","d"],["oct",2],["append"],["append-chord"],["chord"],["member"],["note","e"],["acc",-1],["oct",1],["denom",2],["append"],["member"],["note","g"],["oct",1],["append"],["member"],["note","c"],["oct",2],["append"],["append-chord"],["end-tag"],["tag","\\t",0],["parameter","string","","D7b9",""],["parameter","integer","","13",""],["parameter","integer","dx","2","hs"],["add-tag"],["range"],["chord"],["member"],["note","d"],["oct",1],["denom",2],["append"],["member"],["note","f"],["oct",1],["append"],["member"],["note","b"],["acc",-1],["oct",1],["append"],["append-chord"],["chord"],["member"],["note","c"],["oct",1],["denom",2],["append"],["member"],["note","e"],["acc",-1],["oct",1],["append"],["member"],["note","a"],["acc",-1],["oct",1],["append"],["append-chord"],["end-tag"],["tag","\\t",0],["parameter","string","","Gm7",""],["parameter","integer","","14",""],["add-tag"],["range"],["chord"],["member"],["note","d"],["oct",1],["denom",2],["append"],["member"],["note","f"],["oct",1],["append"],["member"],["note","a"],["oct",1],["append"],["append-chord"],["chord"],["member"],["note","b"],["acc",-1],["oct",0],["denom",2],["append"],["member"],["note","d"],["oct",1],["append"],["member"],["note","g"],["oct",1],["append"],["append-chord"],["chord"],["member"],["note","f"],["oct",1],["denom",2],["append"],["member"],["note","b"],["acc",-1],["oct",1],["append"],["append-chord"],["end-tag"],["tag","\\t",0],["parameter","string","","G7#9",""],["parameter","integer","","15",""],["add-tag"],["range"],["chord"],["member"],["note","e"],["acc",-1],["oct",1],["denom",2],["append"],["member"],["note","g"],["oct",1],["append"],["member"],["note","b"],["acc",-1],["oct",1],["append"],["append-chord"],["end-tag"],["tag","\\t",0],["parameter","string","","Cm7",""],["parameter","integer","","15",""],["add-tag"],["range"],["chord"],["member"],["note","e"],["acc",-1],["oct",1],["denom",2],["append"],["member"],["note","g"],["oct",1],["append"],["append-chord"],["chord"],["member"],["note","f"],["oct",1],["denom",2],["append"],["member"],["note","b"],["acc",-1],["oct",1],["append"],["append-chord"],["end-tag"],["tag","\\t",0],["parameter","string","","F7",""],["parameter","integer","","15",""],["parameter","integer","dx","2","hs"],["add-tag"],["range"],["chord"],["member"],["note","g"],["oct",1],["denom",2],["append"],["member"],["note","d"],["oct",2],["append"],["append-chord"],["chord"],["member"],["note","g"],["oct",1],["denom",2],["append"],["member"],["note","c"],["oct",2],["append"],["member"],["note","f"],["oct",2],["append"],["append-chord"],["end-tag"],["tag","\\t",0],["parameter","string","","BbM7",""],["parameter","integer","","15",""],["add-tag"],["range"],["chord"],["member"],["note","g"],["oct",1],["denom",2],["append"],["member"],["note","c"],["oct",2],["append"],["member"],["note","f"],["oct",2],["append"],["append-chord"],["chord"],["member"],["note","f"],["oct",1],["denom",2],["append"],["member"],["note","a"],["oct",1],["append"],["member"],["note","d"],["oct",2],["append"],["append-chord"],["end-tag"],["tag","\\t",0],["parameter","string","","EbM7",""],["parameter","integer","","13",""],["parameter","integer","dx","2","hs"],["add-tag"],["range"],["chord"],["member"],["note","f"],["oct",1],["denom",2],["append"],["member"],["note","b"],["acc",-1],["oct",1],["append"],["append-chord"],["chord"],["member"],["note","f"],["oct",1],["denom",2],["append"],["member"],["note","b"],["acc",-1],["oct",1],["append"],["member"],["note","d"],["oct",2],["append"],["append-chord"],["end-tag"],["tag","\\t",0],["parameter","string","","Am7b5",""],["parameter","integer","","13",""],["parameter","integer","dx","2","hs"],["add-tag"],["range"],["chord"],["member"],["note","g"],["oct",1],["denom",2],["append"],["member"],["note","c"],["oct",2],["append"],["append-chord"],["chord"],["member"],["note","e"],["acc",-1],["oct",1],["denom",2],["append"],["member"],["note","a"],["oct",1],["append"],["append-chord"],["end-tag"],["tag","\\t",0],["parameter","string","","D7#9",""],["parameter","integer","","13",""],["parameter","integer","dx","2","hs"],["add-tag"],["range"],["chord"],["member"],["note","f"],["oct",1],["denom",2],["append"],["member"],["note","b"],["acc",-1],["oct",1],["append"],["append-chord"],["chord"],["member"],["note","e"],["acc",-1],["oct",1],["append"],["member"],["note","a"],["acc",-1],["oct",1],["append"],["append-chord"],["end-tag"],["tag","\\t",0],["parameter","string","","Gm7",""],["parameter","integer","","13",""],["parameter","integer","dx","2","hs"],["add-tag"],["range"],["chord"],["member"],["note","f"],["oct",1],["append"],["member"],["note","b"],["acc",-1],["oct",1],["append"],["append-chord"],["end-tag"],["tag","\\t",0],["parameter","string","","C7#9",""],["parameter","integer","","13",""],["parameter","integer","dx","2","hs"],["add-tag"],["range"],["chord"],["member"],["note","e"],["acc",-1],["oct",1],["denom",2],["append"],["member"],["note","a"],["acc",-1],["oct",1],["append"],["member"],["note","c"],["oct",2],["append"],["append-chord"],["end-tag"],["tag","\\t",0],["parameter","string","","Fm7",""],["parameter","integer","","15",""],["add-tag"],["range"],["chord"],["member"],["note","e"],["acc",-1],["oct",1],["denom",2],["append"],["member"],["note","a"],["acc",-1],["oct",1],["append"],["append-chord"],["end-tag"],["tag","\\t",0],["parameter","string","","Bb7",""],["parameter","integer","","15",""],["add-tag"],["range"],["chord"],["member"],["note","d"],["oct",1],["denom",2],["append"],["member"],["note","g"],["oct",1],["append"],["member"],["note","b"],["acc",-1],["oct",1],["append"],["append-chord"],["end-tag"],["tag","\\t",0],["parameter","string","","Eb7",""],["parameter","integer","","15",""],["add-tag"],["range"],["chord"],["member"],["note","f"],["oct",1],["denom",2],["append"],["member"],["note","b"],["acc",-1],["oct",1],["append"],["append-chord"],["chord"],["member"],["note","f"],["oct",1],["denom",2],["append"],["member"],["note","c"],["oct",2],["append"],["member"],["note","e"],["acc",-1],["oct",2],["append"],["append-chord"],["end-tag"],["tag","\\t",0],["parameter","string","","Am7b5",""],["parameter","integer","","15",""],["add-tag"],["range"],["chord"],["member"],["note","e"],["acc",-1],["oct",1],["denom",2],["append"],["member"],["note","g"],["oct",1],["append"],["member"],["note","c"],["oct",2],["append"],["append-chord"],["end-tag"],["tag","\\t",0],["parameter","string","","D7#9",""],["parameter","integer","","15",""],["add-tag"],["range"],["chord"],["member"],["note","f"],["denom",2],["append"],["member"],["note","b"],["acc",-1],["oct",1],["append"],["member"],["note","d"],["oct",2],["append"],["append-chord"],["end-tag"],["tag","\\t",0],["parameter","string","","Gm7",""],["parameter","integer","","15",""],["add-tag"],["range"],["chord"],["member"],["note","g"],["oct",1],["enum",1],["append"],["member"],["note","d"],["oct",2],["append"],["member"],["note","f"],["oct",2],["append"],["append-chord"],["end-tag"],["tag","\\t",0],["parameter","string","","G7#9",""],["parameter","integer","","15",""],["add-tag"],["range"],["chord"],["member"],["note","e"],["oct",1],["enum",1],["append"],["member"],["note","g"],["acc",1],["oct",1],["append"],["member"],["note","b"],["oct",1],["append"],["append-chord"],["end-tag"],["end-voice"],["append-voice"],["voice"],["tag","\\staff",0],["parameter","integer","","2",""],["add-tag"],["end-tag"],["tag","\\clef",0],["parameter","string","","f4",""],["add-tag"],["end-tag"],["tag","\\key",0],["parameter","integer","","-2",""],["add-tag"],["end-tag"],["tag","\\meter",0],["parameter","string","","4/4",""],["add-tag"],["end-tag"],["chord"],["member"],["note","f"],["oct",0],["enum",1],["append"],["member"],["note","b"],["acc",-1],["oct",0],["append"],["append-chord"],["chord"],["member"],["note","e"],["acc",-1],["oct",0],["enum",1],["append"],["member"],["note","a"],["oct",0],["append"],["append-chord"],["chord"],["member"],["note","g"],["oct",0],["enum",1],["append"],["member"],["note","c"],["oct",1],["append"],["append-chord"],["chord"],["member"],["note","g"],["oct",0],["enum",1],["append"],["member"],["note","c"],["oct",1],["append"],["append-chord"],["chord"],["member"],["note","a"],["oct",0],["enum",1],["append"],["member"],["note","d"],["oct",1],["append"],["append-chord"],["chord"],["member"],["note","f"],["acc",1],["oct",0],["denom",2],["append"],["member"],["note","c"],["oct",1],["append"],["append-chord"],["chord"],["member"],["note","f"],["acc",1],["oct",0],["denom",2],["append"],["member"],["note","c"],["oct",1],["append"],["append-chord"],["chord"],["member"],["note","g"],["oct",0],["enum",1],["append"],["member"],["note","c"],["oct",1],["append"],["append-chord"],["chord"],["member"],["note","f"],["oct",0],["denom",2],["append"],["member"],["note","b"],["oct",0],["append"],["append-chord"],["chord"],["member"],["note","f"],["oct",0],["denom",2],["append"],["member"],["note","b"],["oct",0],["append"],["append-chord"],["tag","\\repeatEnd",0],["add-tag"],["end-tag"],["chord"],["member"],["note","f"],["acc",1],["oct",0],["denom",2],["append"],["member"],["note","c"],["oct",1],["append"],["append-chord"],["chord"],["member"],["note","f"],["acc",1],["oct",0],["denom",2],["append"],["member"],["note","c"],["oct",1],["append"],["append-chord"],["chord"],["member"],["note","g"],["oct",0],["denom",2],["append"],["member"],["note","c"],["oct",1],["append"],["append-chord"],["chord"],["member"],["note","b"],["acc",-1],["oct",0],["denom",2],["append"],["member"],["note","d"],["oct",1],["append"],["append-chord"],["chord"],["member"],["note","b"],["acc",-1],["oct",0],["denom",2],["append"],["member"],["note","e"],["oct",1],["append"],["append-chord"],["chord"],["member"],["note","d"],["oct",1],["denom",2],["append"],["member"],["note","e"],["oct",1],["append"],["member"],["note","g"],["oct",1],["append"],["append-chord"],["chord"],["member"],["note","g"],["oct",0],["denom",2],["append"],["member"],["note","c"],["oct",1],["append"],["append-chord"],["chord"],["member"],["note","a"],["oct",0],["denom",2],["append"],["member"],["note","d"],["oct",1],["append"],["append-chord"],["chord"],["member"],["note","f"],["acc",1],["oct",0],["denom",2],["append"],["member"],["note","c"],["oct",1],["append"],["append-chord"],["chord"],["member"],["note","f"],["acc",1],["oct",0],["denom",2],["append"],["member"],["note","b"],["acc",-1],["oct",0],["append"],["append-chord"],["chord"],["member"],["note","f"],["oct",0],["denom",2],["append"],["member"],["note","b"],["acc",-1],["oct",0],["append"],["append-chord"],["chord"],["member"],["note","f"],["oct",0],["denom",2],["append"],["member"],["note","a"],["oct",0],["append"],["append-chord"],["chord"],["member"],["note","g"],["oct",0],["denom",2],["append"],["member"],["note","c"],["oct",1],["append"],["append-chord"],["chord"],["member"],["note","f"],["oct",0],["denom",2],["append"],["member"],["note","b"],["oct",0],["append"],["append-chord"],["chord"],["member"],["note","f"],["oct",0],["denom",2],["append"],["member"],["note","b"],["acc",-1],["oct",0],["append"],["append-chord"],["chord"],["member"],["note","g"],["oct",0],["denom",2],["append"],["member"],["note","c"],["oct",1],["append"],["append-chord"],["chord"],["member"],["note","a"],["oct",0],["denom",2],["append"],["member"],["note","e"],["acc",-1],["oct",1],["append"],["append-chord"],["chord"],["member"],["note","e"],["acc",-1],["oct",0],["denom",2],["append"],["member"],["note","a"],["oct",0],["append"],["member"],["note","d"],["oct",1],["append"],["append-chord"],["chord"],["member"],["note","a"],["oct",0],["denom",2],["append"],["member"],["note","d"],["oct",1],["append"],["append-chord"],["chord"],["member"],["note","b"],["acc",-1],["oct",0],["denom",2],["append"],["member"],["note","c"],["oct",1],["append"],["append-chord"],["chord"],["member"],["note","g"],["oct",0],["denom",2],["append"],["member"],["note","c"],["append"],["append-chord"],["chord"],["member"],["note","g"],["oct",0],["denom",2],["append"],["member"],["note","c"],["append"],["append-chord"],["chord"],["member"],["note","a"],["oct",0],["denom",2],["append"],["member"],["note","e"],["acc",-1],["oct",1],["append"],["append-chord"],["chord"],["member"],["note","g"],["oct",0],["denom",2],["append"],["member"],["note","c"],["oct",1],["append"],["append-chord"],["chord"],["member"],["note","f"],["acc",1],["oct",0],["denom",2],["append"],["member"],["note","c"],["oct",1],["append"],["append-chord"],["chord"],["member"],["note","f"],["acc",1],["oct",0],["denom",2],["append"],["member"],["note","c"],["oct",1],["append"],["append-chord"],["chord"],["member"],["note","g"],["oct",0],["denom",2],["append"],["member"],["note","c"],["oct",1],["append"],["append-chord"],["chord"],["member"],["note","e"],["oct",0],["denom",2],["append"],["member"],["note","b"],["acc",-1],["oct",0],["append"],["append-chord"],["chord"],["member"],["note","f"],["oct",0],["denom",2],["append"],["member"],["note","b"],["acc",-1],["oct",0],["append"],["append-chord"],["chord"],["member"],["note","a"],["acc",-1],["oct",0],["denom",2],["append"],["member"],["note","c"],["oct",1],["append"],["append-chord"],["chord"],["member"],["note","g"],["oct",0],["denom",2],["append"],["member"],["note","d"],["acc",-1],["oct",1],["append"],["append-chord"],["chord"],["member"],["note","g"],["oct",0],["denom",2],["append"],["member"],["note","d"],["acc",-1],["oct",1],["append"],["append-chord"],["chord"],["member"],["note","g"],["oct",0],["denom",2],["append"],["member"],["note","c"],["oct",1],["append"],["append-chord"],["chord"],["member"],["note","f"],["acc",1],["oct",0],["denom",2],["append"],["member"],["note","c"],["oct",1],["append"],["append-chord"],["chord"],["member"],["note","f"],["oct",0],["enum",1],["append"],["member"],["note","a"],["oct",0],["append"],["member"],["note","b"],["acc",-1],["oct",0],["append"],["append-chord"],["chord"],["member"],["note","f"],["oct",0],["enum",1],["append"],["member"],["note","b"],["oct",0],["append"],["append-chord"],["end-voice"],["append-voice"],["end-score"]])GMN",
                   ""},
  };
  for (const auto &test : cases) {
    std::ifstream file(std::string(CURSIV_TEST_DIR) + "/native/" + test.file);
    const std::string source((std::istreambuf_iterator<char>(file)), {});
    const auto result = language.language.parser->parse(source);
    const auto *root = cursiv::ast_cast<guido::ast::Prog>(result.value);
    std::ostringstream actual;
    Projection(actual).tree(root);
    bool parents = root && root->parent == nullptr;
    std::unordered_set<const cursiv::AstNode *> seen;
    const auto visit = [&](auto &&self, const cursiv::AstNode &node) -> void {
      if (!seen.insert(&node).second) {
        parents = false;
        return;
      }
      language.language.astReflection->visitMembers(
          node, [&](std::string_view, std::uint32_t, cursiv::AstNode &child) {
            parents &= child.parent == &node &&
                       result.cst().contains(node.cstNodeId, child.cstNodeId);
            self(self, child);
          });
    };
    if (root)
      visit(visit, *root);
    parents &= seen.size() == result.nodes.size();
    const auto validation =
        cursiv::test::validationHelper(language.language, source);
    bool diagnostics = test.code[0] ? validation.diagnostics.size() == 1
                                    : validation.diagnostics.empty();
    if (test.code[0] && validation.diagnostics.size() == 1) {
      const auto &d = validation.diagnostics[0];
      diagnostics &=
          d.code && d.code->isString() && d.code->string() == test.code;
    }
    if (!result.accepted || result.cst().recovered() ||
        !result.diagnostics.empty() || actual.str() != test.expected ||
        !parents || !diagnostics) {
      std::fprintf(stderr, "guido: wrong native fields: %s\n", test.file);
      ++failures;
    }
  }
  for (const auto &test : {std::pair{"[_1]", "guido.rest-pitch"},
                           std::pair{"[c....]", "guido.duration-dots"},
                           std::pair{"[c*-2]", "guido.duration-sign"},
                           std::pair{"[{c d}]", "guido.chord-member"},
                           std::pair{"[\\tag<2zz>]", "guido.parameter-unit"}}) {
    const auto result = language.language.parser->parse(test.first);
    const auto validation =
        cursiv::test::validationHelper(language.language, test.first);
    bool found = false;
    for (const auto &d : validation.diagnostics)
      found |= d.code && d.code->isString() && d.code->string() == test.second;
    if (!result.accepted || result.cst().recovered() ||
        !result.diagnostics.empty() || !found)
      ++failures;
  }
  {
    const auto result =
        language.language.parser->parse("[c<003>+02/8.. \\tag<key=bare>]");
    const auto *root = cursiv::ast_cast<guido::ast::Prog>(result.value);
    const auto *seq =
        root && !root->sequences.empty() ? root->sequences[0] : nullptr;
    const auto *note = seq && !seq->items.empty()
                           ? cursiv::ast_cast<guido::ast::Note>(seq->items[0])
                           : nullptr;
    const auto *tag = seq && seq->items.size() == 2
                          ? cursiv::ast_cast<guido::ast::Tag>(seq->items[1])
                          : nullptr;
    const auto *param =
        tag && tag->parameters.size() == 1 ? tag->parameters[0] : nullptr;
    if (!note || note->annotation != "003" || !note->octave ||
        note->octave->value != "02" || !note->octave->positive ||
        note->dots != ".." || !param || !param->bare || param->name != "key" ||
        param->text != "bare")
      ++failures;
  }
  return failures;
}
} // namespace native_regressions
