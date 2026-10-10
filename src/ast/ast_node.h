#pragma once

#include <string_view>
#include <vector>

#include "literal_type.h"

/**
 * Represents a node in the AST.
 */
class Node {
   public:
    virtual ~Node() = default;
};

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
    ArgumentPair(K key, V value) : key(key), value(value) {}

    const K& getKey() const { return key; }
    const V& getValue() const { return value; }

    virtual ~ArgumentPair() = default;
};