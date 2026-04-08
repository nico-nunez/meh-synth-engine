#include "json/Json.h"

#include <cstdlib>
#include <cstring>
#include <sstream>

namespace json {

// ============================================================
// Value factories
// ============================================================

Value Value::null() {
  return Value{};
}

Value Value::boolean(bool b) {
  Value v;
  v.type = Type::Bool;
  v.boolVal = b;
  return v;
}

Value Value::number(double n) {
  Value v;
  v.type = Type::Number;
  v.numVal = n;
  return v;
}

Value Value::string(const char* s) {
  Value v;
  v.type = Type::String;
  v.strVal = s;
  return v;
}

Value Value::string(std::string s) {
  Value v;
  v.type = Type::String;
  v.strVal = std::move(s);
  return v;
}

Value Value::array() {
  Value v;
  v.type = Type::Array;
  return v;
}

Value Value::object() {
  Value v;
  v.type = Type::Object;
  return v;
}

// ============================================================
// Value accessors
// ============================================================

// Sentinel for failed lookups — returned by reference, never modified.
static const Value NULL_VALUE{};

size_t Value::size() const {
  if (type == Type::Array)
    return arrVals.size();

  if (type == Type::Object)
    return objEntries.size();

  return 0;
}

const Value& Value::at(size_t index) const {
  if (type != Type::Array || index >= arrVals.size())
    return NULL_VALUE;

  return arrVals[index];
}

Value& Value::push(Value val) {
  if (type == Type::Array) {
    arrVals.push_back(std::move(val));
  }
  return *this;
}

const Value& Value::operator[](const char* key) const {
  if (type != Type::Object)
    return NULL_VALUE;

  for (const auto& entry : objEntries) {
    if (entry.first == key)
      return entry.second;
  }
  return NULL_VALUE;
}

bool Value::has(const char* key) const {
  if (type != Type::Object)
    return false;

  for (const auto& entry : objEntries) {
    if (entry.first == key)
      return true;
  }
  return false;
}

Value& Value::set(const char* key, Value val) {
  return set(std::string(key), std::move(val));
}

Value& Value::set(const std::string& key, Value val) {
  if (type != Type::Object)
    return *this;

  for (auto& entry : objEntries) {
    if (entry.first == key) {
      entry.second = std::move(val);
      return *this;
    }
  }
  objEntries.emplace_back(key, std::move(val));
  return *this;
}

Value& Value::getOrCreate(const char* key) {
  if (type != Type::Object)
    return *this;

  for (auto& entry : objEntries)
    if (entry.first == key)
      return entry.second;

  objEntries.push_back({key, Value::object()});
  return objEntries.back().second;
}

// ============================================================
// Parser
// ============================================================

namespace {

struct Parser {
  const char* input;
  size_t pos;
  size_t len;
  uint32_t line;
  uint32_t col;
  std::string error;

  Parser(const char* src, size_t length) : input(src), pos(0), len(length), line(1), col(1) {}

  // ---- Character access ----

  bool atEnd() const { return pos >= len; }

  char peek() const { return atEnd() ? '\0' : input[pos]; }

  char advance() {
    char c = input[pos++];
    if (c == '\n') {
      line++;
      col = 1;
    } else {
      col++;
    }
    return c;
  }

  // ---- Error reporting ----

  void setError(const char* msg) {
    if (error.empty()) {
      std::ostringstream oss;
      oss << "line " << line << ", col " << col << ": " << msg;
      error = oss.str();
    }
  }

  bool hasError() const { return !error.empty(); }

  // ---- Whitespace ----

  void skipWhitespace() {
    while (!atEnd()) {
      char c = peek();
      if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
        advance();
      } else {
        break;
      }
    }
  }

  // ---- Expect a specific character ----

  bool expect(char expected) {
    skipWhitespace();
    if (atEnd() || peek() != expected) {
      char msg[64];
      std::snprintf(msg, sizeof(msg), "expected '%c', got '%c'", expected, atEnd() ? '\0' : peek());
      setError(msg);
      return false;
    }
    advance();
    return true;
  }

  // ---- Parse string ----

  std::string parseString() {
    // Opening quote already consumed or about to be consumed
    if (!expect('"'))
      return {};

    std::string result;
    while (!atEnd()) {
      char c = advance();
      if (c == '"')
        return result;

      if (c == '\\') {
        if (atEnd()) {
          setError("unterminated escape");
          return {};
        }
        char esc = advance();
        switch (esc) {
        case '"':
          result += '"';
          break;
        case '\\':
          result += '\\';
          break;
        case '/':
          result += '/';
          break;
        case 'b':
          result += '\b';
          break;
        case 'f':
          result += '\f';
          break;
        case 'n':
          result += '\n';
          break;
        case 'r':
          result += '\r';
          break;
        case 't':
          result += '\t';
          break;
        case 'u':
          // Skip 4 hex digits — we don't decode Unicode escapes,
          // but we won't choke on them either.
          for (int i = 0; i < 4 && !atEnd(); i++)
            advance();
          result += '?'; // Placeholder
          break;
        default:
          setError("invalid escape character");
          return {};
        }
      } else if (static_cast<unsigned char>(c) < 0x20) {
        setError("control character in string");
        return {};
      } else {
        result += c;
      }
    }

    setError("unterminated string");
    return {};
  }

  // ---- Parse number ----

  double parseNumber() {
    size_t start = pos;

    // Optional minus
    if (peek() == '-')
      advance();

    // Integer part
    if (peek() == '0') {
      advance();
    } else if (peek() >= '1' && peek() <= '9') {
      while (!atEnd() && peek() >= '0' && peek() <= '9')
        advance();
    } else {
      setError("invalid number");
      return 0.0;
    }

    // Fractional part
    if (!atEnd() && peek() == '.') {
      advance();
      if (atEnd() || peek() < '0' || peek() > '9') {
        setError("invalid number: expected digit after '.'");
        return 0.0;
      }
      while (!atEnd() && peek() >= '0' && peek() <= '9')
        advance();
    }

    // Exponent
    if (!atEnd() && (peek() == 'e' || peek() == 'E')) {
      advance();
      if (!atEnd() && (peek() == '+' || peek() == '-'))
        advance();
      if (atEnd() || peek() < '0' || peek() > '9') {
        setError("invalid number: expected digit in exponent");
        return 0.0;
      }
      while (!atEnd() && peek() >= '0' && peek() <= '9')
        advance();
    }

    // Convert the substring
    std::string numStr(input + start, pos - start);
    return std::strtod(numStr.c_str(), nullptr);
  }

  // ---- Parse keyword (true/false/null) ----

  bool matchKeyword(const char* keyword) {
    size_t kwLen = std::strlen(keyword);
    if (pos + kwLen > len)
      return false;
    if (std::strncmp(input + pos, keyword, kwLen) != 0)
      return false;
    // Ensure the keyword isn't a prefix of a longer token
    if (pos + kwLen < len) {
      char next = input[pos + kwLen];
      if ((next >= 'a' && next <= 'z') || (next >= 'A' && next <= 'Z'))
        return false;
    }
    for (size_t i = 0; i < kwLen; i++)
      advance();
    return true;
  }

  // ---- Parse value (recursive entry point) ----

  Value parseValue() {
    skipWhitespace();

    if (atEnd()) {
      setError("unexpected end of input");
      return Value::null();
    }

    char c = peek();

    if (c == '{')
      return parseObject();
    if (c == '[')
      return parseArray();
    if (c == '"') {
      std::string s = parseString();
      return hasError() ? Value::null() : Value::string(std::move(s));
    }
    if (c == 't') {
      if (matchKeyword("true"))
        return Value::boolean(true);
      setError("invalid token (expected 'true')");
      return Value::null();
    }
    if (c == 'f') {
      if (matchKeyword("false"))
        return Value::boolean(false);
      setError("invalid token (expected 'false')");
      return Value::null();
    }
    if (c == 'n') {
      if (matchKeyword("null"))
        return Value::null();
      setError("invalid token (expected 'null')");
      return Value::null();
    }
    if (c == '-' || (c >= '0' && c <= '9')) {
      double n = parseNumber();
      return hasError() ? Value::null() : Value::number(n);
    }

    char msg[64];
    std::snprintf(msg, sizeof(msg), "unexpected character '%c'", c);
    setError(msg);
    return Value::null();
  }

  // ---- Parse object ----

  Value parseObject() {
    advance(); // consume '{'
    Value obj = Value::object();

    skipWhitespace();
    if (!atEnd() && peek() == '}') {
      advance();
      return obj;
    } // empty object

    while (true) {
      skipWhitespace();

      // Key
      if (peek() != '"') {
        setError("expected string key in object");
        return Value::null();
      }
      std::string key = parseString();
      if (hasError())
        return Value::null();

      // Colon
      skipWhitespace();
      if (!expect(':'))
        return Value::null();

      // Value
      Value val = parseValue();
      if (hasError())
        return Value::null();

      obj.set(key, std::move(val));

      // Comma or closing brace
      skipWhitespace();
      if (atEnd()) {
        setError("unterminated object");
        return Value::null();
      }

      if (peek() == '}') {
        advance();
        return obj;
      }
      if (peek() == ',') {
        advance();
        continue;
      }

      setError("expected ',' or '}' in object");
      return Value::null();
    }
  }

  // ---- Parse array ----

  Value parseArray() {
    advance(); // consume '['
    Value arr = Value::array();

    skipWhitespace();
    if (!atEnd() && peek() == ']') {
      advance();
      return arr;
    } // empty array

    while (true) {
      Value val = parseValue();
      if (hasError())
        return Value::null();

      arr.push(std::move(val));

      // Comma or closing bracket
      skipWhitespace();
      if (atEnd()) {
        setError("unterminated array");
        return Value::null();
      }

      if (peek() == ']') {
        advance();
        return arr;
      }
      if (peek() == ',') {
        advance();
        continue;
      }

      setError("expected ',' or ']' in array");
      return Value::null();
    }
  }
};

} // anonymous namespace

ParseResult parse(const std::string& input) {
  ParseResult result;

  if (input.empty()) {
    result.error = "empty input";
    result.errorLine = 1;
    result.errorCol = 1;
    return result;
  }

  Parser parser(input.c_str(), input.size());
  result.value = parser.parseValue();

  if (parser.hasError()) {
    result.error = parser.error;
    result.errorLine = parser.line;
    result.errorCol = parser.col;
    return result;
  }

  // Check for trailing content (besides whitespace)
  parser.skipWhitespace();
  if (!parser.atEnd()) {
    result.error = "unexpected content after value";
    result.errorLine = parser.line;
    result.errorCol = parser.col;
    return result;
  }

  return result;
}

// ============================================================
// Serializer
// ============================================================

namespace {

void serializeValue(const Value& value, std::string& out, bool pretty, int indent);

void writeIndent(std::string& out, int level) {
  for (int i = 0; i < level; i++)
    out += "  ";
}

void serializeString(const std::string& s, std::string& out) {
  out += '"';
  for (char c : s) {
    switch (c) {
    case '"':
      out += "\\\"";
      break;
    case '\\':
      out += "\\\\";
      break;
    case '\b':
      out += "\\b";
      break;
    case '\f':
      out += "\\f";
      break;
    case '\n':
      out += "\\n";
      break;
    case '\r':
      out += "\\r";
      break;
    case '\t':
      out += "\\t";
      break;
    default:
      if (static_cast<unsigned char>(c) < 0x20) {
        char buf[8];
        std::snprintf(buf, sizeof(buf), "\\u%04x", c);
        out += buf;
      } else {
        out += c;
      }
      break;
    }
  }
  out += '"';
}

void serializeNumber(double n, std::string& out) {
  // Integers: no trailing ".0"
  if (n == static_cast<double>(static_cast<int64_t>(n)) && n >= -1e15 && n <= 1e15) {
    out += std::to_string(static_cast<int64_t>(n));
    return;
  }

  char buf[64];
  std::snprintf(buf, sizeof(buf), "%.6g", n);
  out += buf;
}

void serializeValue(const Value& value, std::string& out, bool pretty, int indent) {
  switch (value.type) {
  case Type::Null:
    out += "null";
    break;

  case Type::Bool:
    out += value.boolVal ? "true" : "false";
    break;

  case Type::Number:
    serializeNumber(value.numVal, out);
    break;

  case Type::String:
    serializeString(value.strVal, out);
    break;

  case Type::Array: {
    if (value.arrVals.empty()) {
      out += "[]";
      break;
    }
    out += '[';
    for (size_t i = 0; i < value.arrVals.size(); i++) {
      if (i > 0)
        out += ',';
      if (pretty) {
        out += '\n';
        writeIndent(out, indent + 1);
      } else if (i > 0) {
        out += ' ';
      }
      serializeValue(value.arrVals[i], out, pretty, indent + 1);
    }
    if (pretty) {
      out += '\n';
      writeIndent(out, indent);
    }
    out += ']';
    break;
  }

  case Type::Object: {
    if (value.objEntries.empty()) {
      out += "{}";
      break;
    }
    out += '{';
    for (size_t i = 0; i < value.objEntries.size(); i++) {
      if (i > 0)
        out += ',';
      if (pretty) {
        out += '\n';
        writeIndent(out, indent + 1);
      } else if (i > 0) {
        out += ' ';
      }
      serializeString(value.objEntries[i].first, out);
      out += pretty ? ": " : ":";
      serializeValue(value.objEntries[i].second, out, pretty, indent + 1);
    }
    if (pretty) {
      out += '\n';
      writeIndent(out, indent);
    }
    out += '}';
    break;
  }
  }
}

} // anonymous namespace

std::string serialize(const Value& value, bool pretty) {
  std::string out;
  out.reserve(1024); // Reasonable default for preset files
  serializeValue(value, out, pretty, 0);
  if (pretty)
    out += '\n'; // Trailing newline
  return out;
}

} // namespace json
