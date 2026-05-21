import subprocess
import os
import clang.cindex
from clang.cindex import CursorKind, TypeKind

fieldMapping = {
    'float': 'FieldType::Float',
    'int': 'FieldType::Int',
    'bool': 'FieldType::Bool',
    'std::string': 'FieldType::String',
    'Math::Vec2': 'FieldType::Vec2',
    'Math::Vec3': 'FieldType::Vec3',
    'Math::Vec4': 'FieldType::Vec4',
    'Color': 'FieldType::Color',
    'Axiom::Color': 'FieldType::Color',
    'UUID': 'FieldType::AssetHandle',
    'Axiom::UUID': 'FieldType::AssetHandle',
}

# TODO: make this better
clang.cindex.Config.set_library_file('/Library/Developer/CommandLineTools/usr/lib/libclang.dylib')
try:
    mac_sdk_path = subprocess.check_output(['xcrun', '--show-sdk-path']).decode('utf-8').strip()
    mac_cpp_includes = f"{mac_sdk_path}/usr/include/c++/v1"
    mac_c_includes = f"{mac_sdk_path}/usr/include"
except Exception as e:
    print("Warning: Could not find macOS SDK. C++ standard headers might fail.")
    mac_sdk_path = ""
    mac_cpp_includes = ""
    mac_c_includes = ""

def getFieldType(typeSpelling):
    cleanType = typeSpelling.replace('const ', '').replace('&', '').strip()
    return fieldMapping.get(cleanType, 'FieldType::None')

def generateReflection():
    components = []
    index = clang.cindex.Index.create()

    for root, dirs, files in os.walk('Source/Scene/Components'):
        for file in files:
            if file.endswith('.h'):
                filepath = os.path.join(root, file)
                translationUnit = index.parse(filepath, args=[
                    '-x', 'c++', 
                    '-std=c++23', 
                    '-DAX_REFLECTION_PARSER',
                    '-DAX_PLATFORM_MACOS',
                    '-I', 'Source',
                    '-I', mac_cpp_includes,
                    '-I', mac_c_includes
                ])

                for diag in translationUnit.diagnostics:
                    if diag.severity >= clang.cindex.Diagnostic.Error:
                        print(f"Error in {filepath}: {diag.spelling} at {diag.location}")
                        return

                for node in translationUnit.cursor.walk_preorder():
                    if node.kind in (CursorKind.STRUCT_DECL, CursorKind.CLASS_DECL):
                        annotations = [c.spelling for c in node.get_children() if c.kind == CursorKind.ANNOTATE_ATTR]
                        if "Axiom::Component" in annotations:
                            compName = node.spelling
                            fields = []
                            
                            for child in node.get_children():
                                if child.kind == CursorKind.FIELD_DECL:
                                    fieldAnnotations = [c.spelling for c in child.get_children() if c.kind == CursorKind.ANNOTATE_ATTR]
                                    propertyTag = next((tag for tag in fieldAnnotations if tag.startswith("Axiom::Property")), None)
                                    
                                    if propertyTag:
                                        fieldName = child.spelling
                                        canonType = child.type.get_canonical()
    
                                        parts = propertyTag.split('|')
                                        macroArgs = parts[1].strip() if len(parts) > 1 else ""

                                        assetType = "Axiom::AssetType::None"
                                        optionsStr = "{}"
                                        
                                        if canonType.kind == TypeKind.ENUM:
                                            axType = 'FieldType::Enum'
                                            enumDecl = canonType.get_declaration()
                                            enumOptions = [c.spelling for c in enumDecl.get_children() if c.kind == CursorKind.ENUM_CONSTANT_DECL]
                                            optionsStr = "{" + ", ".join([f'"{opt}"' for opt in enumOptions]) + "}"
                                        else:
                                            axType = getFieldType(child.type.spelling)
                                            if axType == 'FieldType::AssetHandle':
                                                if "AssetType::" in macroArgs:
                                                    assetArg = macroArgs.split(',')[0].strip()
                                                    assetType = "Axiom::" + assetArg
                                                else:
                                                    combinedStr = (child.type.spelling + child.spelling).lower()
                                                    if 'texture' in combinedStr:
                                                        assetType = "Axiom::AssetType::Texture"
                                                    elif 'mesh' in combinedStr or 'model' in combinedStr:
                                                        assetType = "Axiom::AssetType::Mesh"

                                        fields.append((axType, fieldName, optionsStr, assetType))
                            
                            components.append((compName, filepath, fields))

    emptyBrackets = "{}"

    cppCode = "// This file is auto-generated by Reflection.py. Do not edit manually.\n\n"
    cppCode += "#include \"ComponentReflection.h\"\n"

    for compName, filepath, fields in components:
        cleanPath = filepath.replace('\\', '/')
        cppCode += f"#include \"{compName}.h\"\n"

    cppCode += "\nnamespace Axiom {\n"
    cppCode += "std::unordered_map<std::type_index, ComponentInfo> ComponentReflection::componentRegistry;\n\n"

    cppCode += "void ComponentReflection::init() {\n"
    cppCode += "    ComponentInfo info;\n\n"

    for compName, filepath, fields in components:
        cppCode += f"    // Registering {compName} from {filepath}\n"
        cppCode += f"    info.name = \"{compName}\";\n"
        cppCode += f"    info.size = sizeof({compName});\n"
        cppCode += f"    info.fields = {{\n"

        for fieldType, fieldName, optionsStr, assetType in fields:
            cppCode += f"        {{\"{fieldName}\", {fieldType}, offsetof({compName}, {fieldName}), {optionsStr}, {assetType}}},\n"

        cppCode += "    };\n"
        cppCode += f"    componentRegistry[std::type_index(typeid({compName}))] = info;\n\n"

    cppCode += "}\n\n"

    cppCode += "void ComponentReflection::addComponent(Entity entity, const std::string& componentType, void* componentData) {\n"
    for compName, _, fields in components:
        cppCode += f"    if (componentType == \"{compName}\") {{\n"
        cppCode += f"        if (componentData) {{\n"
        cppCode += f"            {compName}* comp = static_cast<{compName}*>(componentData);\n"
        cppCode += f"            entity.addComponent(*comp);\n"
        cppCode += "        } else {\n"
        cppCode += f"            entity.addComponent<{compName}>({emptyBrackets});\n"
        cppCode += "        }\n"
        cppCode += "        return;\n"
        cppCode += "    }\n"
    cppCode += "}\n\n"
    cppCode += "} // namespace Axiom\n"

    outFile = os.path.join('Source/Scene/Components', 'ComponentReflection.cpp')

    if os.path.exists(outFile):
        with open(outFile, 'r', encoding='utf-8') as f:
            existingCode = f.read()
        if existingCode == cppCode:
            print(f"No reflection changes detected. Skipping file write to preserve timestamps.")
            return

    with open(outFile, 'w', encoding='utf-8') as f:
        f.write(cppCode)
    
    print(f"Reflection generated successfully for {len(components)} components.")

if __name__ == "__main__":
    generateReflection()