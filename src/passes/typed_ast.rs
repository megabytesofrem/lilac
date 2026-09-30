use crate::ast::operator::{BinaryOp, UnaryOp};
use crate::ast::types::Type;
use crate::ast::{Literal, Visibility};

#[derive(Debug, Clone, PartialEq)]
pub struct TypedExpr {
    pub kind: TypedExprKind,
    pub ty: Type,
}

impl TypedExpr {
    pub fn new(kind: TypedExprKind, ty: Type) -> Self {
        Self { kind, ty }
    }
}

#[derive(Debug, Clone, PartialEq)]
pub enum TypedExprKind {
    Lit(Literal),
    Ident(String),

    Array(Vec<TypedExpr>),
    Tuple(Vec<TypedExpr>),

    Binary {
        op: BinaryOp,
        lhs: Box<TypedExpr>,
        rhs: Box<TypedExpr>,
    },

    Unary {
        op: UnaryOp,
        expr: Box<TypedExpr>,
    },

    Lambda {
        parameter: String,
        body: Box<TypedExpr>,
    },

    Member {
        object: Box<TypedExpr>,
        member: String,
    },

    SelectorCall {
        target: Box<TypedExpr>,
        selector: String,
        args: Vec<(String, TypedExpr)>,
    },

    Let {
        name: String,
        declared_type: Option<Type>,
        value: Box<TypedExpr>,
        body: Box<TypedExpr>,
    },

    Assign {
        target: Box<TypedExpr>,
        value: Box<TypedExpr>,
    },

    With {
        target: Box<TypedExpr>,
        new_props: Vec<(String, TypedExpr)>,
    },

    For {
        iterator: String,
        iterable: Box<TypedExpr>,
        body: Box<TypedExpr>,
    },

    Until {
        condition: Box<TypedExpr>,
        body: Box<TypedExpr>,
    },

    DoBlock {
        bindings: Vec<TypedLetBinding>,
        exprs: Vec<TypedExpr>,
        trailing_expr: Option<Box<TypedExpr>>,
    },

    Block(Vec<TypedExpr>),
}

#[derive(Debug, Clone, PartialEq)]
pub struct TypedLetBinding {
    pub name: String,
    pub declared_type: Option<Type>,
    pub value: TypedExpr,
}

#[derive(Debug, Clone, PartialEq)]
pub struct TypedParameter {
    pub name: String,
    pub ty: Type,
}

#[derive(Debug, Clone, PartialEq)]
pub struct TypedField {
    pub name: String,
    pub ty: Type,
}

#[derive(Debug, Clone, PartialEq)]
pub struct TypedMessage {
    pub target: String,
    pub selector: String,
    pub keyword_args: Vec<TypedParameter>,
    pub return_type: Option<Type>,
}

#[derive(Debug, Clone, PartialEq)]
pub struct TypedMessageHandler {
    pub message: TypedMessage,
    pub handler_body: TypedExpr,
}

#[derive(Debug, Clone, PartialEq)]
pub enum TypedItem {
    Expr(TypedExpr),

    ClassDef {
        name: String,
        fields: Vec<TypedField>,
    },

    EnumDef {
        name: String,
        variants: Vec<String>,
    },

    FunctionDef {
        name: String,
        params: Vec<TypedParameter>,
        visibility: Visibility,
        return_type: Type,
        body: TypedExpr,
    },

    ProtocolDef {
        name: String,
        messages: Vec<TypedMessage>,
    },

    ProtocolImpl {
        name: String,
        on_target: String,
        message_handlers: Vec<TypedMessageHandler>,
    },
}
