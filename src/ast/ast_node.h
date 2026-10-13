#pragma once

#include <memory>
#include <string_view>
#include <utility>
#include <vector>

/**
 * Represents a node in the AST.
 */
class Node {
   public:
    virtual ~Node() = default;
};

using NodePtr = std::unique_ptr<Node>;

/**
 * Represents a top-level item in the AST.
 */
class Item {
   public:
    virtual ~Item() = default;
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