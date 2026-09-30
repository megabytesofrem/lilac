use winnow::Parser;
use winnow::Result;
use winnow::error::ContextError;
use winnow::token::{any, one_of, take_while};
use winnow::{
    ascii::multispace1,
    combinator::{
        Postfix, alt, delimited, expression, not, opt, preceded, repeat, separated, terminated,
    },
};

use super::{keyword, lexeme, p_type, skip_ws, symbol};

use crate::ast::CallArity;
use crate::ast::DoBlock;
use crate::ast::DoStatement;
use crate::ast::Expr;
use crate::ast::LetBinding;
use crate::ast::Literal;
use crate::ast::types;

const KEYWORDS: &[&str] = &[
    "let",
    "in",
    "for",
    "do",
    "until",
    "if",
    "then",
    "else",
    "with",
    "block",
    "end",
    "true",
    "false",
    "yes",
    "no",
    "struct",
    "enum",
    "protocol",
    "message",
    "implement",
    "on",
];

pub fn p_int_literal<'s>(input: &mut &'s str) -> Result<Literal> {
    fn radix<'s>(prefix: &'static str, radix: u32) -> impl Parser<&'s str, i64, ContextError> {
        preceded(prefix, take_while(1.., move |c: char| c.is_digit(radix)))
            .map(move |s: &str| i64::from_str_radix(s, radix).unwrap())
    }

    lexeme(alt((
        radix("0x", 16),
        radix("0o", 8),
        radix("0b", 2),
        radix("", 10),
    )))
    .map(Literal::Int)
    .parse_next(input)
}

pub fn p_float_literal<'s>(input: &mut &'s str) -> Result<Literal> {
    // Floating-point literal with digits before and after the decimal point.
    lexeme(
        (
            take_while(1.., |c: char| c.is_ascii_digit()),
            '.',
            take_while(1.., |c: char| c.is_ascii_digit()),
        )
            .take(),
    )
    .map(|s: &str| Literal::Float(s.parse::<f64>().unwrap()))
    .parse_next(input)
}

pub fn p_string_literal<'s>(input: &mut &'s str) -> Result<Literal> {
    // String literal enclosed in double quotes: "hello"
    lexeme(delimited('"', take_while(0.., |c: char| c != '"'), '"'))
        .map(|s: &str| Literal::Str(s.to_string()))
        .parse_next(input)
}

pub fn p_char_literal<'s>(input: &mut &'s str) -> Result<Literal> {
    // Character literal: $A
    lexeme(preceded('$', any))
        .map(Literal::Char)
        .parse_next(input)
}

pub fn p_bool_literal<'s>(input: &mut &'s str) -> Result<Literal> {
    // Boolean literal is either true, false, yes, or no (objective c style)
    alt((
        keyword("true").value(true),
        keyword("yes").value(true),
        keyword("false").value(false),
        keyword("no").value(false),
    ))
    .map(Literal::Bool)
    .parse_next(input)
}

pub fn p_literal<'s>(input: &mut &'s str) -> Result<Literal> {
    alt((
        // Float before int, otherwise `1.5` parses as `1`
        p_float_literal,
        p_int_literal,
        p_string_literal,
        p_char_literal,
        p_bool_literal,
    ))
    .parse_next(input)
}

pub fn p_identifier<'s>(input: &mut &'s str) -> Result<String> {
    // Identifier: a sequence of letters, digits, hyphens, or underscores, starting with a letter
    // or underscore, and optionally ending with apostrophes.

    // Valid identifiers: hello, _world, foo-bar, baz_qux, quux'
    lexeme(
        (
            one_of(|c: char| c.is_alphabetic() || c == '_'),
            take_while(0.., |c: char| c.is_alphanumeric() || c == '-' || c == '_'),
            take_while(0.., '\''),
        )
            .take()
            .verify(|s: &str| !KEYWORDS.contains(&s)),
    )
    .map(ToString::to_string)
    .parse_next(input)
}

#[rustfmt::skip]
pub fn p_optionally_typed_identifier<'s>(input: &mut &'s str) -> Result<(String, Option<types::Type>)> {
    // Optionally typed identifier: <identifier>[: <type>]

    lexeme((
        p_identifier,
        opt((symbol(":"), p_type)),
    ))
    .map(|(name, opt_type)| (name, opt_type.map(|(_, ty)| ty)))
    .parse_next(input)
}

pub fn p_array_expr<'s>(input: &mut &'s str) -> Result<Expr> {
    // Array expression enclosed in #[...], with elements separated by commas.
    lexeme((
        symbol("#["),
        separated(0.., p_expr, symbol(",")),
        symbol("]"),
    ))
    .map(|(_, elements, _)| Expr::Array(elements))
    .parse_next(input)
}

pub fn p_tuple_expr<'s>(input: &mut &'s str) -> Result<Expr> {
    // Tuple expression enclosed in #(...), with elements separated by commas.
    // #(1, 2, 3)
    lexeme((
        symbol("#("),
        separated(0.., p_expr, symbol(",")),
        symbol(")"),
    ))
    .map(|(_, elements, _)| Expr::Tuple(elements))
    .parse_next(input)
}

pub fn p_if_expr<'s>(input: &mut &'s str) -> Result<Expr> {
    // If expression: if <condition> then <then_branch> [else <else_branch>]
    lexeme((
        keyword("if"),
        p_expr,
        keyword("then"),
        p_expr,
        opt((keyword("else"), p_expr)),
    ))
    .map(|(_, condition, _, then_branch, else_branch)| {
        let mut branches = vec![condition, then_branch];
        if let Some((_, else_branch)) = else_branch {
            branches.push(else_branch);
        }
        Expr::Block(branches)
    })
    .parse_next(input)
}

pub fn p_let_expr<'s>(input: &mut &'s str) -> Result<Expr> {
    // Let expression: let <optionally_typed_identifier> = <expr> in <expr>
    lexeme((
        keyword("let"),
        p_optionally_typed_identifier,
        symbol("="),
        p_expr,
        keyword("in"),
        p_expr,
    ))
    .map(|(_, (name, opt_type), _, value, _, body)| Expr::Let {
        name,
        opt_type,
        value: Box::new(value),
        body: Box::new(body),
    })
    .parse_next(input)
}

pub fn p_member_expr<'s>(input: &mut &'s str) -> Result<Expr> {
    // Member access expression: object.member
    lexeme((p_expr, symbol("."), p_identifier))
        .map(|(object, _, member)| Expr::Member(Box::new(object), member))
        .parse_next(input)
}

fn p_primary<'s>(input: &mut &'s str) -> Result<Expr> {
    alt((
        // Keyword forms must come before identifiers
        p_do_block,
        p_let_expr,
        p_lambda_expr,
        p_literal.map(Expr::Lit),
        p_identifier.map(Expr::Ident),
        p_array_expr,
        p_tuple_expr,
        delimited(symbol("("), p_expr, symbol(")")),
        // [ ... ] disambiguates a full expression, most commonly a nested message send
        delimited(symbol("["), p_expr, symbol("]")),
    ))
    .parse_next(input)
}

fn p_lambda_expr<'s>(input: &mut &'s str) -> Result<Expr> {
    // Lambda expression: |parameter| body

    // TODO: support multiple parameters in the lambda expression.
    // TODO: support optional type annotations for lambda parameters.
    (symbol("|"), p_identifier, symbol("|"), p_expr)
        .map(|(_, parameter, _, body)| Expr::Lambda {
            parameter,
            body: Box::new(body),
        })
        .parse_next(input)
}

// A single `keyword: argument` part of a keyword message. The argument is a binary
// expression (no keyword messages), so nested keyword sends need [ ... ] to disambiguate.
fn p_keyword_part<'s>(input: &mut &'s str) -> Result<(String, Expr)> {
    (p_identifier, symbol(":"), p_binary_expr)
        .map(|(name, _, value)| (name, value))
        .parse_next(input)
}

// A unary selector is a bare identifier not immediately followed by ':' (which
// would make it the start of a keyword part instead).
fn p_unary_selector<'s>(input: &mut &'s str) -> Result<String> {
    terminated(p_identifier, not(symbol(":"))).parse_next(input)
}

// Smalltalk-style unary message chain: `receiver msg1 msg2 msg3`, left-associative.
// This is what ML-style function application (`f x y z`) has been replaced with.
fn p_unary_chain<'s>(input: &mut &'s str) -> Result<Expr> {
    (
        p_primary,
        repeat(0.., p_unary_selector).fold(
            || Vec::new(),
            |mut selectors: Vec<String>, selector| {
                selectors.push(selector);
                selectors
            },
        ),
    )
        .map(|(receiver, selectors)| {
            selectors
                .into_iter()
                .fold(receiver, |target, selector| Expr::SelectorCall {
                    target: Box::new(target),
                    arity: CallArity::Unary,
                    selector,
                    args: Vec::new(),
                })
        })
        .parse_next(input)
}

pub fn p_block<'s>(input: &mut &'s str) -> Result<Expr> {
    // Block expression enclosed in curly braces, with statements separated by semicolons or whitespace.
    // block
    //  as
    //  bs
    // end

    lexeme((
        keyword("block"),
        separated(0.., p_expr, alt((symbol(";"), multispace1))),
        keyword("end"),
    ))
    .map(|(_, statements, _)| Expr::Block(statements))
    .parse_next(input)
}

pub fn p_let_binding_in_do<'s>(input: &mut &'s str) -> Result<LetBinding> {
    lexeme((
        keyword("let"),
        p_optionally_typed_identifier,
        symbol("<-"),
        p_expr,
    ))
    .map(|(_, (name, opt_type), _, value)| LetBinding {
        name,
        opt_type,
        value,
    })
    .parse_next(input)
}

pub fn p_do_block<'s>(input: &mut &'s str) -> Result<Expr> {
    // Do block expression

    // This is different than a regular block expression because `do` blocks are desugared to monadic bind
    // chains where each `let` binding introduces a new monadic bind.

    // do
    //   let x <- [producer  produce];
    //   let y <- [producer2 produce];
    //   [Console show: [x add: y]]
    // end

    // Desugars to the form:
    //
    // [producer transform: |x|
    //   [producer2 transform: |y|
    //      [Console show: [x add: y]]]
    //
    // NOTE: For obvious reasons, this is not very readable (infact it is less readable than Haskell),
    // so do-blocks should be used frequently to avoid having to write manual bind chains.

    lexeme((
        keyword("do"),
        repeat(
            0..,
            terminated(
                alt((
                    p_let_binding_in_do.map(DoStatement::Binding),
                    p_expr.map(DoStatement::Expression),
                )),
                symbol(";"),
            ),
        )
        .fold(
            || Vec::new(),
            |mut statements, statement| {
                statements.push(statement);
                statements
            },
        ),
        keyword("end"),
    ))
    .map(|(_, statements, _)| {
        let mut bindings = Vec::new();
        let mut exprs = Vec::new();

        for statement in statements {
            match statement {
                DoStatement::Binding(binding) => bindings.push(binding),
                DoStatement::Expression(expr) => exprs.push(expr),
            }
        }

        let trailing_expr = exprs.pop();

        // Construct the DoBlock expression with the collected bindings, expressions,
        // and trailing expression.
        Expr::DoBlock(Box::new(DoBlock {
            bindings,
            exprs,
            trailing_expr,
        }))
    })
    .parse_next(input)
}

// A Smalltalk binary selector: one or more operator characters, e.g. `+`, `==`, `<=`.
// Binary messages all share one precedence level and associate left-to-right, so
// there's no per-operator dispatch table like a Pratt parser would use.
fn p_binary_selector<'s>(input: &mut &'s str) -> Result<String> {
    // TODO: Allow unicode operator characters as well.
    let valid_chars = "+-*/<>=!$%@";

    lexeme(take_while(1.., |c: char| "+-*/<>=!".contains(c)))
        .map(ToString::to_string)
        .parse_next(input)
}

// A unary chain, plus the `with` postfix message (binds tighter than binary messages).
fn p_operand<'s>(input: &mut &'s str) -> Result<Expr> {
    expression(p_unary_chain)
        .postfix(preceded(
            (skip_ws, keyword("with")),
            // <target> with { name: value, ... }
            Postfix(30, |input, target| {
                let new_props: Vec<(String, Expr)> = delimited(
                    symbol("{"),
                    separated(
                        0..,
                        (p_identifier, symbol(":"), p_expr).map(|(name, _, value)| (name, value)),
                        symbol(","),
                    ),
                    symbol("}"),
                )
                .parse_next(input)?;

                Ok(Expr::With {
                    target: Box::new(target),
                    new_props,
                })
            }),
        ))
        .parse_next(input)
}

// Binary message chain, e.g. `a + b - c`. A lone `=` is assignment; every other
// selector (including `==`) is sent as an ordinary binary message.
fn p_binary_expr<'s>(input: &mut &'s str) -> Result<Expr> {
    (
        p_operand,
        repeat(0.., (p_binary_selector, p_operand)).fold(
            || Vec::new(),
            |mut messages: Vec<(String, Expr)>, message| {
                messages.push(message);
                messages
            },
        ),
    )
        .map(|(first, messages)| {
            messages.into_iter().fold(first, |target, (selector, rhs)| {
                if selector == "=" {
                    Expr::Assign {
                        target: Box::new(target),
                        value: Box::new(rhs),
                    }
                } else {
                    Expr::SelectorCall {
                        target: Box::new(target),
                        arity: CallArity::Binary,
                        selector: selector.clone(),
                        args: vec![(selector, rhs)],
                    }
                }
            })
        })
        .parse_next(input)
}

// Top-level expression parser: a keyword message send, e.g. `receiver kw1: a kw2: b`.
// Keyword messages bind the loosest, and their arguments are binary expressions, so a
// nested keyword send as an argument needs [ ... ] to disambiguate where it ends.
pub fn p_expr<'s>(input: &mut &'s str) -> Result<Expr> {
    (
        p_binary_expr,
        repeat(0.., p_keyword_part).fold(
            || Vec::new(),
            |mut parts: Vec<(String, Expr)>, part| {
                parts.push(part);
                parts
            },
        ),
    )
        .map(|(receiver, parts)| {
            if parts.is_empty() {
                receiver
            } else {
                let selector = parts
                    .iter()
                    .map(|(name, _)| format!("{name}:"))
                    .collect::<String>();
                Expr::SelectorCall {
                    arity: CallArity::Keyword,
                    target: Box::new(receiver),
                    selector,
                    args: parts,
                }
            }
        })
        .parse_next(input)
}

#[cfg(test)]
mod tests {
    use super::*;

    fn parse(input: &str) -> Expr {
        let mut input = input;
        let expr = p_expr(&mut input).expect("expression should parse");
        assert!(input.is_empty(), "unparsed input: {input:?}");
        expr
    }

    fn ident(name: &str) -> Expr {
        Expr::Ident(name.to_string())
    }

    fn int(value: i64) -> Expr {
        Expr::Lit(Literal::Int(value))
    }

    fn selector_call(target: Expr, selector: &str, args: Vec<(&str, Expr)>) -> Expr {
        let arity = if args.is_empty() {
            CallArity::Unary
        } else {
            if selector.ends_with(':') {
                CallArity::Keyword
            } else {
                CallArity::Binary
            }
        };

        Expr::SelectorCall {
            target: Box::new(target),
            selector: selector.to_string(),
            arity,
            args: args
                .into_iter()
                .map(|(name, value)| (name.to_string(), value))
                .collect(),
        }
    }

    #[test]
    fn literals_parse() {
        assert_eq!(parse("42"), int(42));
        assert_eq!(parse("1.5"), Expr::Lit(Literal::Float(1.5)));
        assert_eq!(parse("\"hello\""), Expr::Lit(Literal::Str("hello".into())));
        assert_eq!(parse("$A"), Expr::Lit(Literal::Char('A')));
        assert_eq!(parse("true"), Expr::Lit(Literal::Bool(true)));
    }

    #[test]
    fn identifiers_parse() {
        assert_eq!(parse("kebab-case"), ident("kebab-case"));
        assert_eq!(parse("snake_case"), ident("snake_case"));
        assert_eq!(parse("camelCase"), ident("camelCase"));
        assert_eq!(parse("trailing'"), ident("trailing'"));
    }

    #[test]
    fn unary_message_chain_is_left_associative() {
        assert_eq!(
            parse("window contentView frame"),
            selector_call(
                selector_call(ident("window"), "contentView", vec![]),
                "frame",
                vec![]
            )
        );
        assert_eq!(parse("this-as-ident"), ident("this-as-ident"));
    }

    #[test]
    fn unbracketed_keyword_message_send() {
        // window contentView addSubviews: [button withLabel: "Click"]
        assert_eq!(
            parse(r#"window contentView addSubviews: [button withLabel: "Click"]"#),
            selector_call(
                selector_call(ident("window"), "contentView", vec![]),
                "addSubviews:",
                vec![(
                    "addSubviews",
                    selector_call(
                        ident("button"),
                        "withLabel:",
                        vec![("withLabel", Expr::Lit(Literal::Str("Click".into())))]
                    )
                )]
            )
        );
    }

    #[test]
    fn bracketed_keyword_message_send_disambiguates() {
        // [[window contentView] addSubviews:[button withLabel:"Click"]]
        assert_eq!(
            parse(r#"[[window contentView] addSubviews:[button withLabel:"Click"]]"#),
            selector_call(
                selector_call(ident("window"), "contentView", vec![]),
                "addSubviews:",
                vec![(
                    "addSubviews",
                    selector_call(
                        ident("button"),
                        "withLabel:",
                        vec![("withLabel", Expr::Lit(Literal::Str("Click".into())))]
                    )
                )]
            )
        );
    }

    #[test]
    fn arithmetic_respects_precedence() {
        // Smalltalk binary messages have equal precedence and associate left-to-right,
        // so `1 + 2 * 3` is `(1 + 2) * 3`, not `1 + (2 * 3)`.
        assert_eq!(
            parse("1 + 2 * 3"),
            selector_call(
                selector_call(int(1), "+", vec![("+", int(2))]),
                "*",
                vec![("*", int(3))]
            )
        );
        assert_eq!(
            parse("x - 1"),
            selector_call(ident("x"), "-", vec![("-", int(1))])
        );
    }

    #[test]
    fn comparisons_and_assignment_parse() {
        assert_eq!(
            parse("x == 1"),
            selector_call(ident("x"), "==", vec![("==", int(1))])
        );
        assert_eq!(
            parse("x != 1"),
            selector_call(ident("x"), "!=", vec![("!=", int(1))])
        );
        assert!(matches!(parse("x = 1"), Expr::Assign { .. }));
    }

    #[test]
    fn let_for_until_parse() {
        assert!(matches!(parse("let x = f y in x"), Expr::Let { name, .. } if name == "x"));
    }

    #[test]
    fn with_parse() {
        assert!(
            matches!(parse("window with { title: text }"), Expr::With { new_props, .. } if new_props.len() == 1)
        );
    }

    #[test]
    fn do_block_parse() {
        let source = r#"do
    let x: i32 <- [Just value: 5];
    let y: i32 <- [Just value: 6];
    let z: i32 <- [Just value: [x apply: |c| c + 1]];
    [Console show: [z describe]];
end"#;

        assert!(matches!(parse(source), Expr::DoBlock(block)
            if block.bindings.len() == 3
                && block.exprs.is_empty()
                && block.trailing_expr.is_some()));
    }

    #[test]
    fn reserved_words_are_not_identifiers() {
        let mut input = "let for in do";
        assert!(p_identifier(&mut input).is_err());
    }
}
