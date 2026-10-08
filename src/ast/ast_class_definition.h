#pragma once

#include <string_view>
#include <vector>

#include "ast_node.h"
#include "ast_types.h"

/**
 * The method visibility levels for class methods.
 */
enum class ClassMethodVisibility { Public, Private };

struct ClassMethod {
    std::string_view name;
    ClassMethodVisibility visibility;
    Type returnType;
    Node* body;

    ClassMethod(std::string_view name, ClassMethodVisibility visibility,
                Type returnType, Node* body)
        : name(name),
          visibility(visibility),
          returnType(returnType),
          body(body) {}
};

/**
 * Represents a class definition in the AST.
 */
class ClassDefinition : public Item {
   private:
    std::string_view name;
    std::string_view superclass;
    std::vector<std::string_view> conformances;
    std::vector<ClassMethod> methods;

   public:
    ClassDefinition(std::string_view name,
                    const std::vector<ClassMethod>& methods)
        : name(name), methods(methods) {}

   public:
    virtual ~ClassDefinition() = default;
};