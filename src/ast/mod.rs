use std::fmt;

use crate::ast::operator::{BinaryOp, UnaryOp};

pub mod message;
pub mod operator;
pub mod types;

#[derive(Debug, Clone, PartialEq)]
pub enum Literal {
    Int(i64),
    Float(f64),
    Str(String),
    Char(char),
    Bool(bool),
}

#[derive(Debug, Clone, PartialEq)]
pub enum Visibility {
    Public,
    Private,
}

#[derive(Debug, Clone, PartialEq)]
pub enum Expr {
    Lit(Literal),
    Ident(String),

    Array(Vec<Expr>),
    Tuple(Vec<Expr>),

    Binary {
        op: BinaryOp,
        lhs: Box<Expr>,
        rhs: Box<Expr>,
    },

    Unary {
        op: UnaryOp,
        expr: Box<Expr>,
    },

    Lambda {
        parameter: String,
        body: Box<Expr>,
    },

    // Member access expression: object.member
    Member(Box<Expr>, String),

    // ML style function application: f x y z
    Call {
        callee: Box<Expr>,
        arguments: Vec<Expr>,
    },

    // Objective-C style selector call: [target selector-name arg1:val]
    SelectorCall {
        target: Box<Expr>,
        selector: String,
        args: Vec<(String, Expr)>,
    },

    // ML style let expression:
    // let <optionally_typed_identifier> = <expr> in <expr>
    Let {
        name: String,
        opt_type: Option<types::Type>,
        value: Box<Expr>,
        body: Box<Expr>,
    },

    Assign {
        target: Box<Expr>,
        value: Box<Expr>,
    },

    // ML style with expression: <target> with { new_props }
    With {
        target: Box<Expr>,
        new_props: Vec<(String, Expr)>,
    },

    For {
        iterator: String,
        iterable: Box<Expr>,
        body: Box<Expr>,
    },

    Until {
        condition: Box<Expr>,
        body: Box<Expr>,
    },

    DoBlock(Box<DoBlock>),

    Block(Vec<Expr>),
}

#[derive(Debug, Clone, PartialEq)]
pub enum DoStatement {
    Binding(LetBinding),
    Expression(Expr),
}

#[derive(Debug, Clone, PartialEq)]
pub struct LetBinding {
    pub name: String,
    pub opt_type: Option<types::Type>,
    pub value: Expr,
}

#[derive(Debug, Clone, PartialEq)]
pub struct DoBlock {
    // Bindings introduced in the do block
    pub bindings: Vec<LetBinding>,

    // Expressions within the do block
    pub exprs: Vec<Expr>,

    // The trailing expression of the do block, if any.
    // Used for the result of the do block
    pub trailing_expr: Option<Expr>,
}

#[derive(Debug, Clone, PartialEq)]
pub enum Item {
    Expr(Expr),

    StructDef {
        name: String,
        fields: Vec<(String, types::Type)>,
    },

    EnumDef {
        name: String,
        variants: Vec<String>,
    },

    FunctionDef {
        name: String,
        params: Vec<(String, types::Type)>,
        visibility: Visibility,
        return_type: types::Type,
        body: Expr,
    },

    /// A protocol definition containing a name and a list of messages
    /// that the protocol exposes.
    ProtocolDef {
        name: String,
        messages: Vec<message::Message>,
    },

    /// A protocol implementation containing the name of the protocol being implemented,
    /// the target on which it is implemented, and the list of message handlers.
    ProtocolImpl {
        name: String,      // Name of the protocol being implemented
        on_target: String, // The target on which the protocol is being implemented
        message_handlers: Vec<message::MessageHandler>,
    },
}
