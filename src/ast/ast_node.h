#pragma once

#include <memory>
#include <string_view>
#include <utility>
#include <vector>

/**
 * A node in the AST.
 */
class ASTNode {
   public:
    virtual ~ASTNode() = default;
};

/**
 * A smart pointer wrapper around an ASTNode.
 */
using NodePtr = std::unique_ptr<ASTNode>;

/**
 * A top-level item in the AST.
 */
class ASTItem {
   public:
    virtual ~ASTItem() = default;
};

template <typename K, typename V>
class ArgumentPair {
    K key;
    V value;

   public:
    ArgumentPair(K key, V value)
        : key(std::move(key)), value(std::move(value)) {}

    const K& getKey() const { return key; }
    const V& getValue() const { return value; }
};