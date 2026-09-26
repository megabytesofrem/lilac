use winnow::Parser;
use winnow::Result;
use winnow::token::{any, one_of, take_while};
use winnow::{
    ascii::multispace1,
    combinator::{
        Infix, alt, delimited, dispatch, expression, fail, opt, preceded, repeat, separated,
    },
};

use super::{lexeme, p_type, skip_ws, symbol};

use crate::ast::BinaryOp;
use crate::ast::Expr;
use crate::ast::Literal;
use crate::ast::types;

pub fn p_int_literal<'s>(input: &mut &'s str) -> Result<Literal> {
    fn base_int_literal<'s>(input: &mut &'s str) -> Result<i64> {
        take_while(1.., |c: char| c.is_ascii_digit())
            .take()
            .map(|s: &str| s.parse::<i64>().unwrap())
            .parse_next(input)
    }

    fn hex_literal<'s>(input: &mut &'s str) -> Result<i64> {
        lexeme((
            symbol("0x"),
            take_while(1.., |c: char| c.is_ascii_hexdigit()),
        ))
        .take()
        .map(|s: &str| i64::from_str_radix(&s[2..], 16).unwrap())
        .parse_next(input)
    }

    fn octal_literal<'s>(input: &mut &'s str) -> Result<i64> {
        lexeme((
            symbol("0o"),
            take_while(1.., |c: char| c.is_ascii_digit() && c < '8'),
        ))
        .take()
        .map(|s: &str| i64::from_str_radix(&s[2..], 8).unwrap())
        .parse_next(input)
    }

    fn bin_literal<'s>(input: &mut &'s str) -> Result<i64> {
        lexeme((
            symbol("0b"),
            take_while(1.., |c: char| c == '0' || c == '1'),
        ))
        .take()
        .map(|s: &str| i64::from_str_radix(&s[2..], 2).unwrap())
        .parse_next(input)
    }

    // Parse the integer literal using the appropriate base-specific parser.
    alt((hex_literal, bin_literal, octal_literal, base_int_literal))
        .map(Literal::Int)
        .parse_next(input)
}

pub fn p_float_literal<'s>(input: &mut &'s str) -> Result<Literal> {
    // Floating-point literal with digits before and after the decimal point.
    lexeme((
        take_while(1.., |c: char| c.is_ascii_digit()),
        symbol("."),
        take_while(1.., |c: char| c.is_ascii_digit()),
    ))
    .take()
    .map(|s: &str| Literal::Float(s.parse::<f64>().unwrap()))
    .parse_next(input)
}

pub fn p_string_literal<'s>(input: &mut &'s str) -> Result<Literal> {
    // String literal enclosed in double quotes: "hello"
    lexeme((
        symbol("\""),
        take_while(0.., |c: char| c != '"'),
        symbol("\""),
    ))
    .take()
    .map(|s: &str| Literal::Str(s.to_string()))
    .parse_next(input)
}

pub fn p_char_literal<'s>(input: &mut &'s str) -> Result<Literal> {
    // Character literal enclosed in double single quotes: ''a''
    lexeme((
        symbol("''"),
        take_while(1.., |c: char| c != '\''),
        symbol("''"),
    ))
    .take()
    .map(|s: &str| Literal::Char(s.chars().nth(1).unwrap()))
    .parse_next(input)
}

pub fn p_bool_literal<'s>(input: &mut &'s str) -> Result<Literal> {
    // Boolean literal is either true, false, yes, or no (objective c style)
    alt((symbol("true"), symbol("false"), symbol("yes"), symbol("no")))
        .take()
        .map(|s: &str| Literal::Bool(s == "true" || s == "yes"))
        .parse_next(input)
}

pub fn p_literal<'s>(input: &mut &'s str) -> Result<Literal> {
    alt((
        p_int_literal,
        p_float_literal,
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
    lexeme((
        one_of(|c: char| c.is_alphabetic() || c == '_'),
        take_while(0.., |c: char| c.is_alphanumeric() || c == '-' || c == '_'),
        opt(take_while(1.., |c: char| c == '\'')),
    ))
    .take()
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
    // Array expression enclosed in [...], with elements separated by commas.
    lexeme((
        symbol("["),
        separated(0.., p_expr, symbol(",")),
        symbol("]"),
    ))
    .map(|(_, elements, _)| Expr::Array(elements))
    .parse_next(input)
}

pub fn p_set_expr<'s>(input: &mut &'s str) -> Result<Expr> {
    // Set expression enclosed in #[...], with elements separated by commas.
    lexeme((
        symbol("#["),
        separated(0.., p_expr, symbol(",")),
        symbol("]"),
    ))
    .map(|(_, elements, _)| Expr::Set(elements))
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
        symbol("if"),
        p_expr,
        symbol("then"),
        p_expr,
        opt((symbol("else"), p_expr)),
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
        symbol("let"),
        p_optionally_typed_identifier,
        symbol("="),
        p_expr,
        symbol("in"),
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

fn p_application_primary<'s>(input: &mut &'s str) -> Result<Expr> {
    alt((
        p_literal.map(Expr::Lit),
        p_identifier.map(Expr::Ident),
        p_array_expr,
        p_set_expr,
        p_tuple_expr,
        delimited(symbol("("), p_expr, symbol(")")),
    ))
    .parse_next(input)
}

pub fn p_application_expr<'s>(input: &mut &'s str) -> Result<Expr> {
    // ML style function application: f x y z
    (
        p_application_primary,
        repeat(0.., (multispace1, p_application_primary)).fold(
            || Vec::new(),
            |mut arguments: Vec<Expr>, (_, argument)| {
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

    // Objective-C style selector call: tgt.@(method-name arg1:val)
    lexeme((
        symbol("@("),
        p_expr,       // target expression
        p_identifier, // selector name
        separated(0.., parse_keyword_argument, symbol(" ")),
        symbol(")"),
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
    // block
    //  as
    //  bs
    // end

    lexeme((
        symbol("block"),
        separated(0.., p_expr, alt((symbol(";"), multispace1))),
        symbol("end"),
    ))
    .map(|(_, statements, _)| Expr::Block(statements))
    .parse_next(input)
}

// Main expression parser
pub fn p_expr<'s>(input: &mut &'s str) -> Result<Expr> {
    expression(p_application_expr)
        .infix(preceded(
            skip_ws,
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
                '=' => dispatch! { any;
                    '=' => Infix::Left(5, |_, lhs, rhs| {
                        Ok(Expr::Binary {
                            op: BinaryOp::Eq,
                            lhs: Box::new(lhs),
                            rhs: Box::new(rhs),
                        })
                    }),
                    _ => fail,
                },
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
        ))
        .parse_next(input)
}
