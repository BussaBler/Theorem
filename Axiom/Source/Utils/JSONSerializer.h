#pragma once

#include <string>
#include <unordered_map>
#include <vector>

namespace Axiom {
    enum class JSONValueType { Null, Object, Array, String, Float, Int, Bool };

    class JSONValue {
      public:
        JSONValue() : type(JSONValueType::Null) {}
        ~JSONValue() = default;

        inline void setString(const std::string& value) {
            stringValue = value;
            type = JSONValueType::String;
        }
        inline void setFloat(float value) {
            floatValue = value;
            type = JSONValueType::Float;
        }
        inline void setInt(int value) {
            intValue = value;
            type = JSONValueType::Int;
        }
        inline void setBool(bool value) {
            boolValue = value;
            type = JSONValueType::Bool;
        }

        inline void setChild(const std::string& key, const JSONValue& value) {
            children[key] = value;
            type = JSONValueType::Object;
        }

        inline void addArrayElement(const JSONValue& value) {
            elements.push_back(value);
            type = JSONValueType::Array;
        }

        inline JSONValueType getType() const { return type; }
        inline const std::string& getString() const { return stringValue; }
        inline float getFloat() const { return floatValue; }
        inline int getInt() const { return intValue; }
        inline bool getBool() const { return boolValue; }

        inline bool hasChild(const std::string& key) const { return children.find(key) != children.end(); }
        inline const JSONValue& getChild(const std::string& key) const { return children.at(key); }

        inline const std::unordered_map<std::string, JSONValue>& getChildren() const { return children; }
        inline const std::vector<JSONValue>& getArrayElements() const { return elements; }

      private:
        JSONValueType type;

        std::string stringValue;
        float floatValue;
        int intValue;
        bool boolValue;

        std::unordered_map<std::string, JSONValue> children;
        std::vector<JSONValue> elements;
    };

    class JSONSerializer {
      public:
        static std::string serialize(const JSONValue& root);
        static JSONValue deserialize(const std::string& content);

      private:
        static std::string serialize(const JSONValue& value, int indentLevel);

        class JSONParser {
          public:
            JSONParser(const std::string& content) : content(content), position(0) {}
            JSONValue parse() {
                skipWhitespace();
                return parseValue();
            }

          private:
            void skipWhitespace() {
                while (position < content.size() && std::isspace(content[position])) {
                    position++;
                }
            }

            std::string parseRawString() {
                std::string result = "";
                position++;

                while (position < content.length() && content[position] != '"') {
                    result += content[position];
                    position++;
                }

                position++;
                return result;
            }
            JSONValue parseValue() {
                if (position >= content.length()) {
                    return JSONValue();
                }

                char c = content[position];

                if (c == '{') {
                    return parseObject();
                }
                if (c == '[') {
                    return parseArray();
                }
                if (c == '"') {
                    JSONValue val;
                    val.setString(parseRawString());
                    return val;
                }
                if (c == 't') {
                    position += 4;
                    JSONValue val;
                    val.setBool(true);
                    return val;
                }
                if (c == 'f') {
                    position += 5;
                    JSONValue val;
                    val.setBool(false);
                    return val;
                }
                if (c == 'n') {
                    position += 4;
                    return JSONValue();
                }
                if (c == '-' || std::isdigit(c)) {
                    return parseNumber();
                }

                return JSONValue();
            }

            JSONValue parseObject() {
                JSONValue obj;
                position++;
                skipWhitespace();

                while (position < content.length() && content[position] != '}') {
                    std::string key = parseRawString();
                    skipWhitespace();

                    if (content[position] == ':') {
                        position++;
                    }
                    skipWhitespace();

                    obj.setChild(key, parseValue());
                    skipWhitespace();

                    if (content[position] == ',') {
                        position++;
                    }
                    skipWhitespace();
                }

                position++;
                return obj;
            }

            JSONValue parseArray() {
                JSONValue arr;
                position++;
                skipWhitespace();

                while (position < content.length() && content[position] != ']') {
                    arr.addArrayElement(parseValue());
                    skipWhitespace();

                    if (content[position] == ',') {
                        position++;
                    }
                    skipWhitespace();
                }

                position++;
                return arr;
            }

            JSONValue parseNumber() {
                size_t startPos = position;
                bool isFloat = false;
                while (position < content.length() && (std::isdigit(content[position]) || content[position] == '-' || content[position] == '.')) {
                    if (content[position] == '.')
                        isFloat = true;
                    position++;
                }

                std::string numStr = content.substr(startPos, position - startPos);
                JSONValue val;

                if (isFloat) {
                    val.setFloat(std::stof(numStr));
                } else {
                    val.setInt(std::stoi(numStr));
                }
                return val;
            }

          private:
            std::string content;
            size_t position;
        };
    };
} // namespace Axiom
