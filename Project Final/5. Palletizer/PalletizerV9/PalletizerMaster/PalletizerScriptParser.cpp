#include "PalletizerScriptParser.h"
#include "PalletizerMaster.h"

PalletizerScriptParser::PalletizerScriptParser(PalletizerMaster* master)
  : palletizerMaster(master), functionCount(0), yieldInterval(DEFAULT_YIELD_INTERVAL),
    commandCounter(0), lastYieldTime(0) {
}

void PalletizerScriptParser::parseScript(const String& script) {
  String cleanScript = script;
  trimWhitespace(cleanScript);

  if (cleanScript.length() == 0) return;

  resetCommandCounter();
  forceYield();

  int pos = 0;
  while (pos < cleanScript.length()) {
    checkYield();

    int funcPos = cleanScript.indexOf("FUNC(", pos);

    if (funcPos != -1 && (funcPos == 0 || cleanScript.charAt(funcPos - 1) == ' ' || cleanScript.charAt(funcPos - 1) == '\n' || cleanScript.charAt(funcPos - 1) == '\r')) {
      parseFunction(cleanScript, funcPos);

      int openBrace = cleanScript.indexOf('{', funcPos);
      int closeBrace = findMatchingBrace(cleanScript, openBrace);
      pos = closeBrace + 1;

      checkYield();
    } else {
      int nextFunc = cleanScript.indexOf("FUNC(", pos);
      String scriptPart;

      if (nextFunc == -1) {
        scriptPart = cleanScript.substring(pos);
        pos = cleanScript.length();
      } else {
        scriptPart = cleanScript.substring(pos, nextFunc);
        pos = nextFunc;
      }

      scriptPart.trim();
      if (scriptPart.length() > 0) {
        String statements[50];
        int statementCount = 0;
        tokenizeStatements(scriptPart, statements, statementCount);

        for (int i = 0; i < statementCount; i++) {
          statements[i].trim();
          if (statements[i].length() > 0) {
            executeStatement(statements[i]);
            checkYield();
          }
        }
      }
    }
  }
}

void PalletizerScriptParser::executeStatement(const String& statement) {
  String cleanStatement = statement;
  trimWhitespace(cleanStatement);

  if (cleanStatement.length() == 0) return;

  commandCounter++;

  if (cleanStatement.startsWith("CALL(") && cleanStatement.endsWith(")")) {
    String funcName = cleanStatement.substring(5, cleanStatement.length() - 1);
    trimWhitespace(funcName);
    callFunction(funcName);
  } else {
    processSingleStatement(cleanStatement);
  }

  checkYield();
}

bool PalletizerScriptParser::callFunction(const String& funcName) {
  String trimmedName = funcName;
  trimWhitespace(trimmedName);

  for (int i = 0; i < functionCount; i++) {
    if (userFunctions[i].name.equals(trimmedName)) {
      String statements[50];
      int statementCount = 0;
      tokenizeStatements(userFunctions[i].body, statements, statementCount);

      for (int j = 0; j < statementCount; j++) {
        statements[j].trim();
        if (statements[j].length() > 0) {
          executeStatement(statements[j]);
        }
      }
      return true;
    }
  }
  return false;
}

void PalletizerScriptParser::clearFunctions() {
  functionCount = 0;
  for (int i = 0; i < MAX_FUNCTIONS; i++) {
    userFunctions[i].name = "";
    userFunctions[i].body = "";
  }
}

int PalletizerScriptParser::getFunctionCount() {
  return functionCount;
}

String PalletizerScriptParser::getFunctionName(int index) {
  if (index >= 0 && index < functionCount) {
    return userFunctions[index].name;
  }
  return "";
}

void PalletizerScriptParser::setYieldInterval(int interval) {
  if (interval > 0 && interval <= 100) {
    yieldInterval = interval;
  }
}

int PalletizerScriptParser::getYieldInterval() {
  return yieldInterval;
}

void PalletizerScriptParser::resetCommandCounter() {
  commandCounter = 0;
}

int PalletizerScriptParser::getCommandCounter() {
  return commandCounter;
}

void PalletizerScriptParser::parseFunction(const String& script, int startPos) {
  if (functionCount >= MAX_FUNCTIONS) return;

  String funcName = extractFunctionName(script.substring(startPos));
  if (!isValidFunctionName(funcName) || functionExists(funcName)) return;

  String funcBody = extractFunctionBody(script, startPos);
  if (funcBody.length() == 0) return;

  userFunctions[functionCount].name = funcName;
  userFunctions[functionCount].body = funcBody;
  functionCount++;
}

String PalletizerScriptParser::extractFunctionBody(const String& script, int startPos) {
  int openBrace = script.indexOf('{', startPos);
  if (openBrace == -1) return "";

  int closeBrace = findMatchingBrace(script, openBrace);
  if (closeBrace == -1) return "";

  String body = script.substring(openBrace + 1, closeBrace);
  trimWhitespace(body);
  return body;
}

String PalletizerScriptParser::extractFunctionName(const String& funcDef) {
  int startParen = funcDef.indexOf('(');
  int endParen = funcDef.indexOf(')', startParen);

  if (startParen == -1 || endParen == -1) return "";

  String name = funcDef.substring(startParen + 1, endParen);
  trimWhitespace(name);
  return name;
}

void PalletizerScriptParser::tokenizeStatements(const String& input, String* statements, int& count) {
  count = 0;
  String current = "";

  for (int i = 0; i < input.length() && count < 50; i++) {
    char c = input.charAt(i);

    if (c == ';') {
      current.trim();
      if (current.length() > 0) {
        statements[count] = current;
        count++;
      }
      current = "";

      if (count % yieldInterval == 0) {
        checkYield();
      }
    } else {
      current += c;
    }
  }

  current.trim();
  if (current.length() > 0 && count < 50) {
    statements[count] = current;
    count++;
  }
}

void PalletizerScriptParser::processSingleStatement(const String& statement) {
  String cleanStatement = statement;
  trimWhitespace(cleanStatement);

  if (cleanStatement.length() == 0) return;

  if (cleanStatement.startsWith("SET(") || cleanStatement == "WAIT") {
    palletizerMaster->processCommand(cleanStatement);
  } else if (cleanStatement == "ZERO" || cleanStatement == "IDLE" || cleanStatement == "PLAY" || cleanStatement == "PAUSE" || cleanStatement == "STOP") {
    palletizerMaster->processCommand(cleanStatement);
  } else if (cleanStatement.startsWith("SPEED;")) {
    palletizerMaster->processCommand(cleanStatement);
  } else if (cleanStatement.indexOf('(') != -1 && (cleanStatement.startsWith("X(") || cleanStatement.startsWith("Y(") || cleanStatement.startsWith("Z(") || cleanStatement.startsWith("T(") || cleanStatement.startsWith("G("))) {
    palletizerMaster->processCommand(cleanStatement);
  } else if (cleanStatement.indexOf(',') != -1) {
    palletizerMaster->processCommand(cleanStatement);
  } else if (cleanStatement.startsWith("GRIPPER;") || cleanStatement.startsWith("TOOL;")) {
    palletizerMaster->processCommand(cleanStatement);
  }
}

int PalletizerScriptParser::findMatchingBrace(const String& script, int openPos) {
  if (openPos == -1 || openPos >= script.length()) return -1;

  int braceCount = 1;
  for (int i = openPos + 1; i < script.length(); i++) {
    if (script.charAt(i) == '{') {
      braceCount++;
    } else if (script.charAt(i) == '}') {
      braceCount--;
      if (braceCount == 0) {
        return i;
      }
    }

    if (i % 100 == 0) {
      checkYield();
    }
  }
  return -1;
}

void PalletizerScriptParser::trimWhitespace(String& str) {
  str.trim();

  while (str.length() > 0 && (str.charAt(0) == ' ' || str.charAt(0) == '\t' || str.charAt(0) == '\n' || str.charAt(0) == '\r')) {
    str = str.substring(1);
  }

  while (str.length() > 0 && (str.charAt(str.length() - 1) == ' ' || str.charAt(str.length() - 1) == '\t' || str.charAt(str.length() - 1) == '\n' || str.charAt(str.length() - 1) == '\r')) {
    str = str.substring(0, str.length() - 1);
  }
}

bool PalletizerScriptParser::isValidFunctionName(const String& name) {
  if (name.length() == 0) return false;

  for (int i = 0; i < name.length(); i++) {
    char c = name.charAt(i);
    if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_')) {
      return false;
    }
  }
  return true;
}

bool PalletizerScriptParser::functionExists(const String& name) {
  for (int i = 0; i < functionCount; i++) {
    if (userFunctions[i].name.equals(name)) {
      return true;
    }
  }
  return false;
}

void PalletizerScriptParser::checkYield() {
  if (commandCounter > 0 && commandCounter % yieldInterval == 0) {
    forceYield();
  }
}

void PalletizerScriptParser::forceYield() {
  unsigned long currentTime = millis();
  if (currentTime - lastYieldTime >= DEFAULT_YIELD_DELAY) {
    yield();
    delay(DEFAULT_YIELD_DELAY);
    lastYieldTime = currentTime;
  }
}