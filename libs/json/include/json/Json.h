#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace json {

// ============================================================
// Value type
// ============================================================

enum class Type : uint8_t { Null, Bool, Number, String, Array, Object };

struct Value {
  Type type = Type::Null;

  // Storage — only one is meaningful based on `type`.
  // All are always default-constructed. For a preset file with a few hundred
  // values, the memory overhead is negligible and the simplicity is worth it.
  bool boolVal = false;
  double numVal = 0.0;
  std::string strVal;
  std::vector<Value> arrVals;
  std::vector<std::pair<std::string, Value>> objEntries;

  // ---- Construction (named factories for clarity) ----

  static Value null();
  static Value boolean(bool b);
  static Value number(double n);
  static Value string(const char* s);
  static Value string(std::string s);
  static Value array();
  static Value object();

  // ---- Type checks ----

  bool isNull() const { return type == Type::Null; }
  bool isBool() const { return type == Type::Bool; }
  bool isNumber() const { return type == Type::Number; }
  bool isString() const { return type == Type::String; }
  bool isArray() const { return type == Type::Array; }
  bool isObject() const { return type == Type::Object; }

  // ---- Accessors (return default if wrong type — no exceptions) ----

  bool asBool() const { return isBool() ? boolVal : false; }
  double asNumber() const { return isNumber() ? numVal : 0.0; }
  float asFloat() const { return isNumber() ? static_cast<float>(numVal) : 0.0f; }
  int32_t asInt() const { return isNumber() ? static_cast<int32_t>(numVal) : 0; }
  int8_t asInt8() const { return isNumber() ? static_cast<int8_t>(numVal) : 0; }
  const std::string& asString() const { return strVal; } // empty if not string

  // ---- Array access ----

  // Returns number of elements (array) or entries (object). 0 for other types.
  size_t size() const;

  // Array index access. Returns null Value if out of bounds or wrong type.
  const Value& at(size_t index) const;

  // Append to array. No-op if not an array.
  Value& push(Value val);

  // ---- Object access ----

  // Key lookup. Returns null Value if key not found or wrong type.
  const Value& operator[](const char* key) const;

  // Check if key exists. False if not an object.
  bool has(const char* key) const;

  // Set key-value pair. Overwrites if key exists. No-op if not an object.
  Value& set(const char* key, Value val);
  Value& set(const std::string& key, Value val);

  // Returns a mutable reference to the child object at key,
  // creating it as an empty object if it doesn't exist.
  Value& getOrCreate(const char* key);
};

// ============================================================
// Parsing
// ============================================================

struct ParseResult {
  Value value;
  std::string error;      // empty on success
  uint32_t errorLine = 0; // 1-based line number
  uint32_t errorCol = 0;  // 1-based column number

  bool ok() const { return error.empty(); }
};

// Parse a JSON string. Returns ParseResult with value on success,
// or error message with line/column on failure.
ParseResult parse(const std::string& input);

// ============================================================
// Serialization
// ============================================================

// Serialize a Value to a JSON string.
// pretty=true: indented with newlines (human-readable).
// pretty=false: compact single-line output.
std::string serialize(const Value& value, bool pretty = true);

} // namespace json
