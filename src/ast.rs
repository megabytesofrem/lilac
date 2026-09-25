#[derive(Debug, Clone, PartialEq)]
pub enum Literal {
    Int(i64),
    Float(f64),
    Str(String),
    Char(char),
    Bool(bool),
}

#[derive(Debug, Clone, PartialEq)]
pub enum BinaryOp {
    Add,
    Sub,
    Mul,
    Div,
    Eq,
    Ne,
    Lt,
    Gt,
    Le,
    Ge,
}

#[derive(Debug, Clone, PartialEq)]
pub enum Expr {
    Lit(Literal),
    Ident(String),
    Array(Vec<Expr>),
    Set(Vec<Expr>),
    Tuple(Vec<Expr>),

    Binary {
        op: BinaryOp,
        lhs: Box<Expr>,
        rhs: Box<Expr>,
    },

    // Member access expression: object.member
    Member(Box<Expr>, String),

    // Objective-C style selector call: tgt.@(method-name arg1:val)
    SelectorCall(Box<Expr>, Vec<(String, Expr)>),
    Call(Box<Expr>, Vec<Expr>),

    Block(Vec<Expr>),
}
