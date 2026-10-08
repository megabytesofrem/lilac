#pragma once

#include "ast_node.h"

class ExprItem : public Item {
   private:
    Node* expression;

   public:
    ExprItem(Node* expression) : expression(expression) {}

   public:
    virtual ~ExprItem() = default;
};
