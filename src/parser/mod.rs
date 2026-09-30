mod expression;
mod item;

use winnow::Parser;
use winnow::Result;
use winnow::ascii::{multispace0, multispace1};
use winnow::combinator::delimited;
use winnow::combinator::{alt, not, opt, repeat, separated, terminated};
use winnow::error::ContextError;
use winnow::token::{literal, one_of, take_while};

use crate::ast::types;

pub fn skip_ws<'s>(input: &mut &'s str) -> Result<()> {
    loop {
        let _ = multispace0.parse_next(input)?;
        if input.starts_with("--") {
            let _ = winnow::ascii::till_line_ending.parse_next(input)?;
        }

        // If the input does not start with a comment or whitespace, break the loop -
        // skip comments and whitespace
        if !input.starts_with("--") && !input.starts_with(char::is_whitespace) {
            break;
        }
    }

    Ok(())
}

pub fn lexeme<'s, O, P>(parser: P) -> impl Parser<&'s str, O, ContextError>
where
    P: Parser<&'s str, O, ContextError>,
{
    (parser, skip_ws).map(|(value, _)| value)
}

pub fn symbol<'s>(value: &'static str) -> impl Parser<&'s str, &'s str, ContextError> {
    lexeme(literal(value))
}

// Like `symbol`, but won't match a prefix of a longer identifier (`in` vs `inner`)
pub fn keyword<'s>(value: &'static str) -> impl Parser<&'s str, &'s str, ContextError> {
    lexeme(terminated(
        literal(value),
        not(one_of(|c: char| {
            c.is_alphanumeric() || c == '-' || c == '_' || c == '\''
        })),
    ))
}

#[rustfmt::skip]
pub fn params<'s>(input: &mut &'s str) -> Result<Vec<(String, types::Type)>> {
    fn param<'s>(input: &mut &'s str) -> Result<(String, types::Type)> {
        lexeme((
            opt(symbol("(")),
            expression::p_identifier,
            symbol(":"),
            p_type,
            opt(symbol(")")),
        ))
        .map(|(_, name, _, ty, _)| (name, ty))
        .parse_next(input)
    }

    repeat(0.., param).parse_next(input)
}

fn p_named_type<'s>(input: &mut &'s str) -> Result<types::Type> {
    // Named type starting with a uppercase letter, and followed by alphanumeric characters,
    // hyphens and underscores.
    lexeme(
        (
            one_of(|c: char| c.is_alphabetic() && c.is_uppercase()),
            take_while(0.., |c: char| c.is_alphanumeric() || c == '-' || c == '_'),
            take_while(0.., '\''),
        )
            .take(),
    )
    .map(|s: &str| types::Type::Named(s.to_string()))
    .parse_next(input)
}

// Type parser
pub fn p_type<'s>(input: &mut &'s str) -> Result<types::Type> {
    alt((
        symbol("i8").map(|_| types::Type::I8),
        symbol("i16").map(|_| types::Type::I16),
        symbol("i32").map(|_| types::Type::I32),
        symbol("i64").map(|_| types::Type::I64),
        symbol("u8").map(|_| types::Type::U8),
        symbol("u16").map(|_| types::Type::U16),
        symbol("u32").map(|_| types::Type::U32),
        symbol("u64").map(|_| types::Type::U64),
        symbol("f32").map(|_| types::Type::F32),
        symbol("f64").map(|_| types::Type::F64),
        symbol("bool").map(|_| types::Type::Bool),
        symbol("char").map(|_| types::Type::Char),
        keyword("string").map(|_| types::Type::Str),
        symbol("str").map(|_| types::Type::Str),
        symbol("unit").map(|_| types::Type::Unit),
        // User-defined types
        p_named_type,
        // Array type
        lexeme((symbol("#["), p_type, symbol("]")))
            .map(|(_, ty, _)| types::Type::Array(Box::new(ty))),
        // Tuple type
        lexeme((
            symbol("#("),
            separated(0.., p_type, symbol(",")),
            symbol(")"),
        ))
        .map(|(_, types, _)| types::Type::Tuple(types)),
        // Pointer type
        lexeme((symbol("*"), p_type)).map(|(_, ty)| types::Type::Pointer(Box::new(ty))),
    ))
    .parse_next(input)
}
