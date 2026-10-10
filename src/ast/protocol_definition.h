#pragma once

#include "ast_node.h"
#include "ast_types.h"

class ProtocolDefinition : public Item {
   private:
    std::string_view name;
    std::vector<Message> messages;

   public:
    ProtocolDefinition(std::string_view name,
                       const std::vector<Message>& messages)
        : name(name), messages(messages) {}

    std::string_view getName() const { return name; }
    const std::vector<Message>& getMessages() const { return messages; }

   public:
    virtual ~ProtocolDefinition() = default;
};

class ProtocolImplementation : public Item {
   private:
    std::string_view protocolName;
    std::string_view targetName;
    std::vector<MessageHandler> handlers;

   public:
    ProtocolImplementation(std::string_view protocolName,
                           std::string_view targetName,
                           const std::vector<MessageHandler>& handlers)
        : protocolName(protocolName),
          targetName(targetName),
          handlers(handlers) {}

    std::string_view getProtocolName() const { return protocolName; }
    std::string_view getTargetName() const { return targetName; }
    const std::vector<MessageHandler>& getHandlers() const { return handlers; }

   public:
    virtual ~ProtocolImplementation() = default;
};