use winnow::Parser;
use winnow::Result;
use winnow::combinator::alt;
use winnow::combinator::{opt, preceded, repeat, separated};

use crate::ast::Item;
use crate::ast::message;
use crate::ast::types;
use crate::parser::expression::{p_expr, p_identifier};
use crate::parser::params;

use super::{keyword, lexeme, p_type, symbol};

#[rustfmt::skip]
pub fn struct_field<'s>(input: &mut &'s str) -> Result<(String, types::Type)> {
    lexeme((
        p_identifier,
        symbol(":"),
        p_type,
    ))
    .map(|(name, _, ty)| (name, ty))
    .parse_next(input)
}

#[rustfmt::skip]
pub fn enum_variant<'s>(input: &mut &'s str) -> Result<String> {
    lexeme(p_identifier).parse_next(input)
}

#[rustfmt::skip]
pub fn p_struct_def<'s>(input: &mut &'s str) -> Result<Item> {
    // Struct definition
    // struct <name>
    //     <field_name>: <field_type>,
    //     ...
    // end

    lexeme((
        keyword("struct"),
        p_identifier,
        separated(0.., struct_field, symbol(",")),
        keyword("end"),
    ))
    .map(|(_, name, fields, _)| Item::StructDef { name, fields })
    .parse_next(input)
}

#[rustfmt::skip]
pub fn p_enum_def<'s>(input: &mut &'s str) -> Result<Item> {
    // Enum definition
    // enum <name>
    //     <variant_name>,
    //     ...
    // end

    lexeme((
        keyword("enum"),
        p_identifier,
        separated(0.., enum_variant, symbol(",")),
        keyword("end"),
    ))
    .map(|(_, name, variants, _)| Item::EnumDef { name, variants })
    .parse_next(input)
}

// Protocol

fn p_message_signature<'s>(input: &mut &'s str) -> Result<message::Message> {
    // Message signature definition
    // message <selector> (<params>)

    (keyword("message"), p_identifier, params)
        .map(|(_, selector, args)| message::Message {
            selector: message::Selector(selector),
            target: args
                .first()
                .map(|(name, _)| name.clone())
                .unwrap_or_default(),
            keyword_args: args,
            return_type: None,
        })
        .parse_next(input)
}

fn p_message<'s>(input: &mut &'s str) -> Result<message::Message> {
    // Message signature definition
    // message <selector> (<params>) -> <return_type>

    (p_message_signature, symbol("->"), p_type)
        .map(|(message, _, ty)| message::Message {
            return_type: Some(ty),
            ..message
        })
        .parse_next(input)
}

pub fn p_message_handler<'s>(input: &mut &'s str) -> Result<message::MessageHandler> {
    // Message handler definition
    // message <selector> (<params>) = <handler_body>

    (
        p_message_signature,
        opt(preceded(symbol("->"), p_type)),
        symbol("="),
        p_expr,
    )
        .map(
            |(message, return_type, _, handler_body)| message::MessageHandler {
                message: message::Message {
                    return_type,
                    ..message
                },
                handler_body,
            },
        )
        .parse_next(input)
}

#[rustfmt::skip]
pub fn p_protocol_def<'s>(input: &mut &'s str) -> Result<Item> {
    // Protocol definition
    // protocol <name>
    //     message <selector> (<params>) -> <return_type>
    //     ...
    // end

    lexeme((
        keyword("protocol"),
        p_identifier,
        repeat(0.., p_message),
        keyword("end"),
    ))
    .map(|(_, name, messages, _)| Item::ProtocolDef { name, messages })
    .parse_next(input)
}

#[rustfmt::skip]
pub fn p_implementation<'s>(input: &mut &'s str) -> Result<Item> {
    // Implementation definition
    // implement <protocol_name> on <struct_name>
    //     message <selector> (<params>) = <handler_body>
    //     ...
    // end

    lexeme((
        keyword("implement"),
        p_identifier, // Protocol name to implement
        keyword("on"),
        p_identifier, // Struct on which the protocol is implemented
        repeat(0.., p_message_handler), // Message handlers
        keyword("end"),
    ))
    .map(|(_, name, _, target, handlers, _)| Item::ProtocolImpl {
        name,
        on_target: target,
        message_handlers: handlers,
    })
    .parse_next(input)
}

#[rustfmt::skip]
pub fn p_item<'s>(input: &mut &'s str) -> Result<Item> {
    // Item can be an enum, protocol, or protocol implementation

    alt((
        p_enum_def,         // Enum definition
        p_protocol_def,     // Protocol definition
        p_implementation,   // Protocol implementation
    ))
    .parse_next(input)
}
