#pragma once

#include <utility>
#include <vector>

#include "ast_node.h"
#include "ast_types.h"

class ProtocolDefinition : public ASTItem {
   private:
    std::string_view _name;
    std::vector<Message> _messages;

   public:
    ProtocolDefinition(std::string_view name, std::vector<Message> messages)
        : _name(name), _messages(std::move(messages)) {}

    std::string_view getName() const { return _name; }
    const std::vector<Message>& getMessages() const { return _messages; }

   public:
    virtual ~ProtocolDefinition() = default;
};

class ProtocolImplementation : public ASTItem {
   private:
    std::string_view _protocolName;
    std::string_view _targetName;
    std::vector<MessageHandler> _handlers;

   public:
    ProtocolImplementation(std::string_view protocolName,
                           std::string_view targetName,
                           std::vector<MessageHandler> handlers)
        : _protocolName(protocolName),
          _targetName(targetName),
          _handlers(std::move(handlers)) {}

    std::string_view getProtocolName() const { return _protocolName; }
    std::string_view getTargetName() const { return _targetName; }
    const std::vector<MessageHandler>& getHandlers() const { return _handlers; }

   public:
    virtual ~ProtocolImplementation() = default;
};