#pragma once

#include <string_view>
#include <utility>
#include <vector>

#include "ast_node.h"
#include "ast_types.h"

/**
 * The method visibility levels for class methods.
 */
enum class ClassMethodVisibility { Public, Private };

/**
 * A class method in the AST.
 */
struct ClassMethod {
    std::string_view name;
    ClassMethodVisibility visibility;
    Type returnType;
    NodePtr body;

    ClassMethod(std::string_view name,
                ClassMethodVisibility visibility,
                Type returnType,
                NodePtr body)
        : name(name),
          visibility(visibility),
          returnType(returnType),
          body(std::move(body)) {}

    std::string_view getName() const { return name; }
    ClassMethodVisibility getVisibility() const { return visibility; }
    Type getReturnType() const { return returnType; }
    Node* getBody() const { return body.get(); }
};

/**
 * A class definition in the AST.
 */
class ClassDefinition : public Item {
   private:
    std::string_view name;
    std::string_view superclass;
    std::vector<std::string_view> conformances;
    std::vector<ClassMethod> methods;

   public:
    ClassDefinition(std::string_view name, std::vector<ClassMethod> methods)
        : name(name), methods(std::move(methods)) {}

    std::string_view getName() const { return name; }
    std::string_view getSuperclass() const { return superclass; }
    const std::vector<std::string_view>& getConformances() const {
        return conformances;
    }
    const std::vector<ClassMethod>& getMethods() const { return methods; }

   public:
    virtual ~ClassDefinition() = default;
};