#ifndef PALLETIZER_SCRIPT_PARSER_H
#define PALLETIZER_SCRIPT_PARSER_H

#include "Arduino.h"

class PalletizerMaster;

class PalletizerScriptParser {
public:
  struct Function {
    String name;
    String body;
  };

  PalletizerScriptParser(PalletizerMaster* master);
  void parseScript(const String& script);
  void executeStatement(const String& statement);
  bool callFunction(const String& funcName);
  void clearFunctions();
  int getFunctionCount();
  String getFunctionName(int index);
  void setYieldInterval(int interval);
  int getYieldInterval();
  void resetCommandCounter();
  int getCommandCounter();

private:
  static const int MAX_FUNCTIONS = 20;
  static const int DEFAULT_YIELD_INTERVAL = 5;
  static const int DEFAULT_YIELD_DELAY = 1;

  PalletizerMaster* palletizerMaster;
  Function userFunctions[MAX_FUNCTIONS];
  int functionCount;
  int yieldInterval;
  int commandCounter;
  unsigned long lastYieldTime;

  void parseFunction(const String& script, int startPos);
  String extractFunctionBody(const String& script, int startPos);
  String extractFunctionName(const String& funcDef);
  void tokenizeStatements(const String& input, String* statements, int& count);
  void processSingleStatement(const String& statement);
  int findMatchingBrace(const String& script, int openPos);
  void trimWhitespace(String& str);
  bool isValidFunctionName(const String& name);
  bool functionExists(const String& name);
  void checkYield();
  void forceYield();
};

#endif