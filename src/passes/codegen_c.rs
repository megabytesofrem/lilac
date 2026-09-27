use std::{collections::HashMap, fmt};

use crate::ast;

pub struct CodegenCtx {
    emitted_code: String,
    indentation_level: usize,
}

#[derive(Debug, Clone)]
#[allow(dead_code)]
#[rustfmt::skip]
pub enum CType {
    Int8,       // int8_t
    Int16,      // int16_t
    Int32,      // int32_t
    Int64,      // int64_t
    UInt8,      // uint8_t
    UInt16,     // uint16_t
    UInt32,     // uint32_t
    UInt64,     // uint64_t
    Float,      // float
    Double,     // double
    Bool,       // _Bool
    Char,       // char
    String,     // char*
    Void,       // void

    // Named type
    Named(String),

    Array(Box<CType>),
    Pointer(Box<CType>),
}

impl fmt::Display for CType {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            CType::Int8 => write!(f, "int8_t"),
            CType::Int16 => write!(f, "int16_t"),
            CType::Int32 => write!(f, "int32_t"),
            CType::Int64 => write!(f, "int64_t"),
            CType::UInt8 => write!(f, "uint8_t"),
            CType::UInt16 => write!(f, "uint16_t"),
            CType::UInt32 => write!(f, "uint32_t"),
            CType::UInt64 => write!(f, "uint64_t"),
            CType::Float => write!(f, "float"),
            CType::Double => write!(f, "double"),
            CType::Bool => write!(f, "_Bool"),
            CType::Char => write!(f, "char"),
            CType::String => write!(f, "char*"),
            CType::Void => write!(f, "void"),
            CType::Named(name) => write!(f, "{}", name),
            CType::Array(inner) => write!(f, "{}[]", inner),
            CType::Pointer(inner) => write!(f, "{}*", inner),
        }
    }
}

// MARK: C AST

#[derive(Debug, Clone)]
#[allow(dead_code)]
#[rustfmt::skip]
pub enum AttributeTag {
    Static,         // static
    Inline,         // inline
    Const,          // const
    Volatile,       // volatile
    Align(u32),     // alignas(n)
}

impl fmt::Display for AttributeTag {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            AttributeTag::Static => write!(f, "static"),
            AttributeTag::Inline => write!(f, "inline"),
            AttributeTag::Const => write!(f, "const"),
            AttributeTag::Volatile => write!(f, "volatile"),
            AttributeTag::Align(n) => write!(f, "alignas({})", n),
        }
    }
}

#[derive(Debug, Clone)]
pub enum CPrePostfixOp {
    Inc, // ++
    Dec, // --
}

/// A C function prototype
#[derive(Debug, Clone)]
pub struct CPrototype {
    name: String,
    attributes: Vec<AttributeTag>,
    parameters: Vec<(String, CType)>,
    return_type: CType,
    is_static: bool,
    is_variadic: bool,
}

#[derive(Debug, Clone)]
pub struct CFunction {
    prototype: CPrototype,
    body: Vec<CStmt>,
}

#[derive(Debug, Clone)]
pub struct CHeader {
    includes: Vec<String>,
    macros: HashMap<String, String>,
    prototypes: Vec<CPrototype>,
}

#[derive(Debug, Clone)]
pub struct CImplementation {
    functions: Vec<CFunction>,
}

#[derive(Debug, Clone)]
pub enum CExpr {
    Literal(ast::Literal),
    Ident(String),

    Binary {
        op: ast::BinaryOp,
        left: Box<CExpr>,
        right: Box<CExpr>,
    },

    Unary {
        op: ast::UnaryOp,
        expr: Box<CExpr>,
    },

    Prefix {
        op: CPrePostfixOp,
        expr: Box<CExpr>,
    },

    Postfix {
        op: CPrePostfixOp,
        expr: Box<CExpr>,
    },

    Call {
        callee: Box<CExpr>,
        arguments: Vec<CExpr>,
    },

    /// lilac_msg_send(target, SEL("write-to-file-named:encoding"), ..)
    LilacMessageSend {
        target: Box<CExpr>,
        selector: String,
        arguments: Vec<(String, CExpr)>,
    },
}

#[derive(Debug, Clone)]
pub enum CStmt {
    Expr(CExpr),

    VarDecl {
        name: String,
        ctype: CType,
        initializer: Option<CExpr>,
    },

    Assign {
        target: Box<CExpr>,
        value: Box<CExpr>,
    },

    Return(Option<CExpr>),

    For {
        init: Option<Box<CStmt>>,
        condition: Option<CExpr>,
        increment: Option<CExpr>,
        body: Box<CStmt>,
    },

    While {
        condition: CExpr,
        body: Box<CStmt>,
    },
}

// MARK: C code emission

impl CodegenCtx {
    pub fn new() -> Self {
        Self {
            emitted_code: String::new(),
            indentation_level: 0,
        }
    }

    fn emit_prototype(&mut self, prototype: &CPrototype) {
        // Implementation for emitting C function prototype goes here
    }

    fn emit_header(&mut self, header: &CHeader) {
        // Implementation for emitting C header code goes here
    }

    fn emit_implementation(&mut self, implementation: &CImplementation) {
        // Implementation for emitting C implementation code goes here
    }
}
