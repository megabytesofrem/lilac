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
    std::string_view _name;
    ClassMethodVisibility _visibility;
    Type _returnType;
    NodePtr _body;

    ClassMethod(std::string_view name,
                ClassMethodVisibility visibility,
                Type returnType,
                NodePtr body)
        : _name(name),
          _visibility(visibility),
          _returnType(returnType),
          _body(std::move(body)) {}

    std::string_view getName() const { return _name; }
    ClassMethodVisibility getVisibility() const { return _visibility; }
    Type getReturnType() const { return _returnType; }
    ASTNode* getBody() const { return _body.get(); }
};

/**
 * A class definition in the AST.
 */
class ClassDefinition : public ASTItem {
   private:
    std::string_view _name;
    std::string_view _superclass;
    std::vector<std::string_view> _conformances;
    std::vector<ClassMethod> _methods;

   public:
    ClassDefinition(std::string_view name, std::vector<ClassMethod> methods)
        : _name(name), _methods(std::move(methods)) {}

    std::string_view getName() const { return _name; }
    std::string_view getSuperclass() const { return _superclass; }

    const std::vector<std::string_view>& getConformances() const {
        return _conformances;
    }
    const std::vector<ClassMethod>& getMethods() const { return _methods; }

   public:
    virtual ~ClassDefinition() = default;
};