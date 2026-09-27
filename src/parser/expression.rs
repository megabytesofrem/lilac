use winnow::Parser;
use winnow::Result;
use winnow::error::ContextError;
use winnow::token::{any, one_of, take_while};
use winnow::{
    ascii::multispace1,
    combinator::{
        Infix, Postfix, alt, delimited, dispatch, expression, fail, opt, preceded, repeat,
        separated, terminated,
    },
};

use super::{keyword, lexeme, p_type, skip_ws, symbol};

use crate::ast::Expr;
use crate::ast::Literal;
use crate::ast::operator::BinaryOp;
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
    // Character literal enclosed in double single quotes: ''a''
    lexeme(delimited("''", any, "''"))
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

pub fn p_for_expr<'s>(input: &mut &'s str) -> Result<Expr> {
    // For expression: for <iterator> in <iterable> do <body>
    lexeme((
        keyword("for"),
        p_identifier,
        keyword("in"),
        p_expr,
        keyword("do"),
        p_expr,
    ))
    .map(|(_, iterator, _, iterable, _, body)| Expr::For {
        iterator,
        iterable: Box::new(iterable),
        body: Box::new(body),
    })
    .parse_next(input)
}

#[rustfmt::skip]
pub fn p_until_expr<'s>(input: &mut &'s str) -> Result<Expr> {
    // Until expression: until <condition> do <body>
    lexeme((
        keyword("until"),
        p_expr,
        keyword("do"),
        p_expr,
    ))
    .map(|(_, condition, _, body)| Expr::Until {
        condition: Box::new(condition),
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

fn p_application_primary<'s>(input: &mut &'s str) -> Result<Expr> {
    alt((
        // Keyword forms must come before identifiers
        p_let_expr,
        p_for_expr,
        p_until_expr,
        p_selector_call,
        p_literal.map(Expr::Lit),
        p_identifier.map(Expr::Ident),
        p_array_expr,
        p_tuple_expr,
        delimited(symbol("("), p_expr, symbol(")")),
    ))
    .parse_next(input)
}

pub fn p_application_expr<'s>(input: &mut &'s str) -> Result<Expr> {
    // ML style function application: f x y z
    // Every primary is a lexeme, so arguments separated by whitespace are already adjacent here.
    (
        p_application_primary,
        repeat(0.., p_application_primary).fold(
            || Vec::new(),
            |mut arguments: Vec<Expr>, argument| {
                arguments.push(argument);
                arguments
            },
        ),
    )
        .map(|(callee, arguments)| {
            arguments
                .into_iter()
                .fold(callee, |callee, argument| Expr::Call {
                    callee: Box::new(callee),
                    arguments: vec![argument],
                })
        })
        .parse_next(input)
}

pub fn p_selector_call<'s>(input: &mut &'s str) -> Result<Expr> {
    fn parse_keyword_argument<'s>(input: &mut &'s str) -> Result<(String, Expr)> {
        lexeme((p_identifier, symbol(":"), p_expr))
            .map(|(name, _, value)| (name, value))
            .parse_next(input)
    }

    // Objective-C style selector call: [target selector-name arg1:val1 arg2:val2 ...]
    lexeme((
        symbol("["),
        p_application_primary, // target expression
        p_identifier,          // selector name
        separated(0.., parse_keyword_argument, symbol(" ")),
        symbol("]"),
    ))
    .map(|(_, target, selector, args, _)| Expr::SelectorCall {
        target: Box::new(target),
        selector,
        args,
    })
    .parse_next(input)
}

pub fn p_block<'s>(input: &mut &'s str) -> Result<Expr> {
    // Block expression enclosed in curly braces, with statements separated by semicolons or whitespace.
    // do
    //  as
    //  bs
    // end

    lexeme((
        keyword("do"),
        separated(0.., p_expr, alt((symbol(";"), multispace1))),
        keyword("end"),
    ))
    .map(|(_, statements, _)| Expr::Block(statements))
    .parse_next(input)
}

// Main expression parser
pub fn p_expr<'s>(input: &mut &'s str) -> Result<Expr> {
    expression(p_application_expr)
        .infix(preceded(
            skip_ws,
            terminated(
                dispatch! { any;
                    '+' => Infix::Left(10, |_, lhs, rhs| {
                        Ok(Expr::Binary {
                            op: BinaryOp::Add,
                            lhs: Box::new(lhs),
                            rhs: Box::new(rhs),
                        })
                    }),
                    '-' => Infix::Left(10, |_, lhs, rhs| {
                        Ok(Expr::Binary {
                            op: BinaryOp::Sub,
                            lhs: Box::new(lhs),
                            rhs: Box::new(rhs),
                        })
                    }),
                    '*' => Infix::Left(20, |_, lhs, rhs| {
                        Ok(Expr::Binary {
                            op: BinaryOp::Mul,
                            lhs: Box::new(lhs),
                            rhs: Box::new(rhs),
                        })
                    }),
                    '/' => Infix::Left(20, |_, lhs, rhs| {
                        Ok(Expr::Binary {
                            op: BinaryOp::Div,
                            lhs: Box::new(lhs),
                            rhs: Box::new(rhs),
                        })
                    }),
                    '=' => alt((
                        '='.value(Infix::Left(5, |_, lhs, rhs| {
                            Ok(Expr::Binary {
                                op: BinaryOp::Eq,
                                lhs: Box::new(lhs),
                                rhs: Box::new(rhs),
                            })
                        })),
                        Infix::Right(1, |_, target, value| {
                            Ok(Expr::Assign {
                                target: Box::new(target),
                                value: Box::new(value),
                            })
                        }),
                    )),
                    '!' => dispatch! { any;
                        '=' => Infix::Left(5, |_, lhs, rhs| {
                            Ok(Expr::Binary {
                                op: BinaryOp::Ne,
                                lhs: Box::new(lhs),
                                rhs: Box::new(rhs),
                            })
                        }),
                        _ => fail,
                    },
                    '<' => dispatch! { any;
                        '=' => Infix::Left(5, |_, lhs, rhs| {
                            Ok(Expr::Binary {
                                op: BinaryOp::Le,
                                lhs: Box::new(lhs),
                                rhs: Box::new(rhs),
                            })
                        }),
                        _ => Infix::Left(5, |_, lhs, rhs| {
                            Ok(Expr::Binary {
                                op: BinaryOp::Lt,
                                lhs: Box::new(lhs),
                                rhs: Box::new(rhs),
                            })
                        }),
                    },
                    '>' => dispatch! { any;
                        '=' => Infix::Left(5, |_, lhs, rhs| {
                            Ok(Expr::Binary {
                                op: BinaryOp::Ge,
                                lhs: Box::new(lhs),
                                rhs: Box::new(rhs),
                            })
                        }),
                        _ => Infix::Left(5, |_, lhs, rhs| {
                            Ok(Expr::Binary {
                                op: BinaryOp::Gt,
                                lhs: Box::new(lhs),
                                rhs: Box::new(rhs),
                            })
                        }),
                    },
                    _ => fail,
                },
                skip_ws,
            ),
        ))
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

    fn call(callee: Expr, argument: Expr) -> Expr {
        Expr::Call {
            callee: Box::new(callee),
            arguments: vec![argument],
        }
    }

    fn selector_call(target: Expr, selector: &str, args: Vec<(&str, Expr)>) -> Expr {
        Expr::SelectorCall {
            target: Box::new(target),
            selector: selector.to_string(),
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
        assert_eq!(parse("''a''"), Expr::Lit(Literal::Char('a')));
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
    fn application_is_left_associative() {
        assert_eq!(
            parse("f x y"),
            call(call(ident("f"), ident("x")), ident("y"))
        );
        assert_eq!(parse("this-as-ident"), ident("this-as-ident"));
    }

    #[test]
    fn selector_call_parse() {
        assert_eq!(
            parse("[obj do-something arg1:val1]"),
            selector_call(ident("obj"), "do-something", vec![("arg1", ident("val1"))])
        );
    }

    #[test]
    fn arithmetic_respects_precedence() {
        assert_eq!(
            parse("1 + 2 * 3"),
            Expr::Binary {
                op: BinaryOp::Add,
                lhs: Box::new(int(1)),
                rhs: Box::new(Expr::Binary {
                    op: BinaryOp::Mul,
                    lhs: Box::new(int(2)),
                    rhs: Box::new(int(3)),
                }),
            }
        );
        assert_eq!(
            parse("x - 1"),
            Expr::Binary {
                op: BinaryOp::Sub,
                lhs: Box::new(ident("x")),
                rhs: Box::new(int(1)),
            }
        );
    }

    #[test]
    fn comparisons_and_assignment_parse() {
        assert!(matches!(
            parse("x == 1"),
            Expr::Binary {
                op: BinaryOp::Eq,
                ..
            }
        ));
        assert!(matches!(
            parse("x != 1"),
            Expr::Binary {
                op: BinaryOp::Ne,
                ..
            }
        ));
        assert!(matches!(parse("x = 1"), Expr::Assign { .. }));
    }

    #[test]
    fn let_for_until_parse() {
        assert!(matches!(parse("let x = f y in x"), Expr::Let { name, .. } if name == "x"));
        assert!(matches!(parse("for i in xs do i"), Expr::For { iterator, .. } if iterator == "i"));
        assert!(matches!(parse("until done do step"), Expr::Until { .. }));
    }

    #[test]
    fn with_parse() {
        assert!(
            matches!(parse("window with { title: text }"), Expr::With { new_props, .. } if new_props.len() == 1)
        );
    }

    #[test]
    fn reserved_words_are_not_identifiers() {
        let mut input = "let for in do";
        assert!(p_identifier(&mut input).is_err());
    }
}
