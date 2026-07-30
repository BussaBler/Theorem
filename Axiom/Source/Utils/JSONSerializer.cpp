#include "axpch.h"

#include "JSONSerializer.h"

namespace Axiom {
    std::string JSONSerializer::serialize(const JSONValue& root) {
        return serialize(root, 0);
    }

    JSONValue JSONSerializer::deserialize(const std::string& content) {
        JSONParser parser(content);
        return parser.parse();
    }

    std::string JSONSerializer::serialize(const JSONValue& node, int indentLevel) {
        std::string result = "";
        std::string indent(indentLevel * 4, ' ');
        std::string indentChild((indentLevel + 1) * 4, ' ');

        switch (node.getType()) {
        case JSONValueType::Null:
            result += "null";
            break;
        case JSONValueType::String:
            result += "\"" + node.getString() + "\"";
            break;
        case JSONValueType::Float:
            result += std::to_string(node.getFloat());
            break;
        case JSONValueType::Int:
            result += std::to_string(node.getInt());
            break;
        case JSONValueType::Bool:
            result += node.getBool() ? "true" : "false";
            break;
        case JSONValueType::Object:
            result += "{\n";
            for (const auto& [key, child] : node.getChildren()) {
                result += indentChild + "\"" + key + "\": " + serialize(child, indentLevel + 1) + ",\n";
            }
            result.pop_back();
            result.pop_back();
            result += "\n" + indent + "}";
            break;
        case JSONValueType::Array:
            result += "[\n";
            for (const auto& element : node.getArrayElements()) {
                result += indentChild + serialize(element, indentLevel + 1) + ",\n";
            }
            result.pop_back();
            result.pop_back();
            result += "\n" + indent + "]";
            break;

        default:
            break;
        }
        return result;
    }
} // namespace Axiom
