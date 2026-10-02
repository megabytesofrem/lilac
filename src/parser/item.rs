use winnow::Parser;
use winnow::Result;
use winnow::combinator::alt;
use winnow::combinator::{delimited, opt, preceded, repeat, terminated};

use crate::ast::ClassMethod;
use crate::ast::Item;
use crate::ast::Visibility;
use crate::ast::message;
use crate::ast::types;
use crate::parser::expression::{p_expr, p_identifier};
use crate::parser::params;

use super::{keyword, lexeme, p_type, symbol};

enum ClassMember {
    Field((String, types::Type)),
    Method(ClassMethod),
    OverrideMethod(ClassMethod),
}

pub fn class_name<'s>(input: &mut &'s str) -> Result<String> {
    // Parse a class name: System/greet
    (
        p_identifier,
        repeat(0.., preceded(symbol("/"), p_identifier)),
    )
        .map(|(first, rest): (String, Vec<String>)| {
            std::iter::once(first)
                .chain(rest)
                .collect::<Vec<_>>()
                .join("/")
        })
        .parse_next(input)
}

#[rustfmt::skip]
pub fn class_field<'s>(input: &mut &'s str) -> Result<(String, types::Type)> {
    lexeme((
        p_identifier,
        symbol(":"),
        p_type,
    ))
    .map(|(name, _, ty)| (name, ty))
    .parse_next(input)
}

pub fn class_method<'s>(input: &mut &'s str) -> Result<ClassMethod> {
    // Class method definition
    // def <method_name> (<params>) -> <return_type> = <body>

    lexeme((
        keyword("def"),
        p_identifier,
        params,
        opt(preceded(symbol("->"), p_type)),
        symbol("="),
        p_expr,
    ))
    .map(|(_, name, params, return_type, _, body)| ClassMethod {
        name,
        params,
        visibility: Visibility::Public, // Adjust as needed
        return_type: return_type.unwrap_or(types::Type::Unit),
        body,
    })
    .parse_next(input)
}

fn class_member<'s>(input: &mut &'s str) -> Result<ClassMember> {
    alt((
        class_method.map(ClassMember::Method),
        preceded(keyword("override"), class_method).map(ClassMember::OverrideMethod),
        class_field.map(ClassMember::Field),
    ))
    .parse_next(input)
}

fn class_members<'s>(input: &mut &'s str) -> Result<Vec<ClassMember>> {
    repeat(0.., terminated(class_member, opt(symbol(",")))).parse_next(input)
}

#[rustfmt::skip]
pub fn enum_variant<'s>(input: &mut &'s str) -> Result<String> {
    lexeme(p_identifier).parse_next(input)
}

pub fn p_class_def<'s>(input: &mut &'s str) -> Result<Item> {
    // Class definition
    // class <name> [: super_class] [conforms <protocol_name, protocol_name, ...>] {
    //     <field_name>: <field_type>,
    //
    //     def <method_name> (<params>) -> <return_type> = <body>
    //     override <method_name> (<params>) -> <return_type> = <body>
    // }

    lexeme((
        keyword("class"),
        p_identifier,
        // Optional superclass follows the class name
        opt(preceded(symbol(":"), p_identifier)),
        // Optional conforms clause follows the optional superclass
        opt(preceded(
            keyword("conforms"),
            repeat(0.., terminated(p_identifier, opt(symbol(",")))),
        )),
        alt((
            delimited(symbol("{"), class_members, symbol("}")),
            terminated(
                repeat(0.., terminated(class_field, opt(symbol(",")))),
                symbol("}"),
            )
            .map(|fields: Vec<(String, types::Type)>| {
                fields
                    .into_iter()
                    .map(ClassMember::Field)
                    .collect::<Vec<ClassMember>>()
            }),
        )),
    ))
    .map(|(_, name, superclass_opt, conforms_opt, members)| {
        let mut superclass = None;
        let mut conforms = Vec::new();

        let mut fields = Vec::new();
        let mut methods = Vec::new();
        let mut override_methods = Vec::new();

        if let Some(sc) = superclass_opt {
            superclass = Some(sc);
        }

        if let Some(c) = conforms_opt {
            conforms = c;
        }

        for member in members {
            match member {
                ClassMember::Field(field) => fields.push(field),
                ClassMember::Method(method) => methods.push(method),
                ClassMember::OverrideMethod(method) => override_methods.push(method),
            }
        }

        Item::ClassDef {
            name,
            superclass,
            conforms,
            fields,
            methods,
            override_methods,
        }
    })
    .parse_next(input)
}

#[rustfmt::skip]
pub fn p_enum_def<'s>(input: &mut &'s str) -> Result<Item> {
    // Enum definition
    // enum <name> {
    //     <variant_name>,
    //     ...
    // }

    lexeme((
        keyword("enum"),
        p_identifier,
        delimited(symbol("{"), repeat(0.., terminated(enum_variant, opt(symbol(",")))), symbol("}")),
    ))
    .map(|(_, name, variants)| Item::EnumDef { name, variants })
    .parse_next(input)
}

// Protocol

fn p_message_signature<'s>(input: &mut &'s str) -> Result<message::Message> {
    // Message signature definition
    // <selector> (<params>)

    (p_identifier, params)
        .map(|(selector, args)| message::Message {
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

    (
        keyword("message"),
        p_message_signature,
        symbol("->"),
        p_type,
    )
        .map(|(_, message, _, ty)| message::Message {
            return_type: Some(ty),
            ..message
        })
        .parse_next(input)
}

pub fn p_message_handler<'s>(input: &mut &'s str) -> Result<message::MessageHandler> {
    // Message handler definition
    // message <selector> (<params>) = <handler_body>

    (
        keyword("message"),
        p_message_signature,
        opt(preceded(symbol("->"), p_type)),
        symbol("="),
        p_expr,
    )
        .map(
            |(_, message, return_type, _, handler_body)| message::MessageHandler {
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
    // protocol <name> {
    //     message <selector> (<params>) -> <return_type>
    //     ...
    // }

    lexeme((
        keyword("protocol"),
        p_identifier,
        delimited(symbol("{"), repeat(0.., p_message), symbol("}")),
    ))
    .map(|(_, name, messages)| Item::ProtocolDef { name, messages })
    .parse_next(input)
}

pub fn p_item<'s>(input: &mut &'s str) -> Result<Item> {
    alt((
        p_class_def,    // Class definition
        p_enum_def,     // Enum definition
        p_protocol_def, // Protocol definition
    ))
    .parse_next(input)
}

mod tests {
    #[test]
    fn class_def() {
        let mut input = "class MyClass { field1: i32 }";
        let result = super::p_item(&mut input).unwrap();
        assert!(matches!(
            result,
            crate::ast::Item::ClassDef { fields, methods, .. }
                if fields.len() == 1 && methods.is_empty()
        ));
    }

    #[test]
    fn class_def_exact_dice_syntax() {
        let mut input = r#"class Dice {
    sides: Int
    number: Int

    def makeDice withSides: Int -> Dice = 
        Dice { sides: withSides, number: 0 }.

    def rollDice self:Dice -> IO Dice = do {
        let randNumber <- Random from: 0 to: sides.
        IO pure value: Dice { sides: self sides, number: randNumber }.
    }
}
"#;

        let result = super::p_item(&mut input).unwrap();
        assert!(input.is_empty());
        assert!(matches!(
                result,
                crate::ast::Item::ClassDef { fields, methods, .. }
                        if fields.len() == 2
                                && methods.len() == 2
                                && methods[0].name == "makeDice"
                                && methods[1].name == "rollDice"
        ));
    }

    #[test]
    fn class_def_with_method() {
        let mut input = "class MyClass {
            field1: i32
            def greet who: String -> String = who.
        }";
        let result = super::p_item(&mut input).unwrap();
        assert!(matches!(
            result,
            crate::ast::Item::ClassDef { fields, methods, .. }
                if fields.len() == 1
                    && methods.len() == 1
                    && methods[0].name == "greet"
        ));
    }

    #[test]
    fn enum_def() {
        let mut input = "enum MyEnum {
            Variant1,
            Variant2
        }";
        let result = super::p_item(&mut input);
        assert!(result.is_ok());
    }

    #[test]
    fn protocol_def() {
        let mut input = "protocol MyProtocol {
            message mySelector (arg1: i32) -> bool
        }";
        let result = super::p_item(&mut input);
        assert!(result.is_ok());
    }
}
