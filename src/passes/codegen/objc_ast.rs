//! Objective-C AST definitions for the Lilac compiler

use std::fmt;

use crate::ast;

#[derive(Debug, Clone)]
#[allow(dead_code)]
#[rustfmt::skip]
pub enum CType {
    Id,         // id: generic Objective-C object
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
    Bool,       // BOOL
    Char,       // char
    String,     // NSString *
    Void,       // void

    // Named type
    Named(String),

    // NS-prefixed types e.g NSString, NSNumber
    NS(String),

    Array(Box<CType>),
    Pointer(Box<CType>),

    // Objective-C block type: return_type (^)(parameter_types)
    Block(Box<CType>, Vec<CType>),
}

impl fmt::Display for CType {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            CType::Id => write!(f, "id"),
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
            CType::Bool => write!(f, "BOOL"),
            CType::Char => write!(f, "char"),
            CType::String => write!(f, "NSString *"),
            CType::Void => write!(f, "void"),
            CType::Named(name) => write!(f, "{}", name),
            CType::NS(name) => write!(f, "{}", name),
            CType::Array(inner) => write!(f, "{}[]", inner),
            CType::Pointer(inner) => write!(f, "{} *", inner),
            CType::Block(return_type, parameters) => {
                let params: Vec<String> = parameters.iter().map(|p| format!("{}", p)).collect();
                write!(f, "{} (^)( {} )", return_type, params.join(", "))
            }
        }
    }
}

// MARK: Objective C AST

/// A tag for an Objective-C property attribute.
#[derive(Debug, Clone)]
#[allow(dead_code)]
#[rustfmt::skip]
pub enum AttributeTag {
    Nonatomic,      // @property (nonatomic)
    Weak,           // @property (weak)
    Strong,         // @property (strong)
    Assign,         // @property (assign)
    ReadOnly,       // @property (readonly)
    ReadWrite,      // @property (readwrite)
    Copy,           // @property (copy)
    Getter,         // @property (getter)
    Setter,         // @property (setter)
}

impl fmt::Display for AttributeTag {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            AttributeTag::Nonatomic => write!(f, "nonatomic"),
            AttributeTag::Weak => write!(f, "weak"),
            AttributeTag::Strong => write!(f, "strong"),
            AttributeTag::Assign => write!(f, "assign"),
            AttributeTag::ReadOnly => write!(f, "readonly"),
            AttributeTag::ReadWrite => write!(f, "readwrite"),
            AttributeTag::Copy => write!(f, "copy"),
            AttributeTag::Getter => write!(f, "getter"),
            AttributeTag::Setter => write!(f, "setter"),
        }
    }
}

/// An Objective-C property
///
/// Contains the name of the property, its attribute tags, and its type.
#[derive(Debug, Clone)]
pub struct ObjCPropertyField {
    pub name: String,
    pub attribute_tags: Vec<AttributeTag>,
    pub ctype: CType,
}

/// An Objective-C method prototype.
#[derive(Debug, Clone)]
pub struct ObjCPrototype {
    pub name: String,
    pub parameters: Vec<(String, CType)>,
    pub return_type: CType,
    pub is_static_method: bool,
}

/// An Objective-C method
///
/// NOTE: If the method is static, it will be prefixed with a `+` rather than a `-`.
#[derive(Debug, Clone)]
pub struct ObjCMethod {
    pub prototype: ObjCPrototype,
    pub body: Vec<ObjCStmt>,
}

/// An Objective-C C struct.
///
/// Contains the name of the struct and its fields.
#[derive(Debug, Clone)]
pub struct ObjCCStruct {
    pub name: String,
    pub fields: Vec<ObjCPropertyField>,
}

/// An Objective-C C enum.
///
/// Contains the name of the enum and its variants.
#[derive(Debug, Clone)]
pub struct ObjCCEnum {
    pub name: String,
    pub variants: Vec<String>,
}

/// An Objective-C @interface
///
/// Contains the name of the interface, its subclass, properties, and methods.
///
/// ```objc
/// // Person.h
/// @interface Person : NSObject
///
/// @property (nonatomic, strong) NSString *name;
///
/// - (void)sayHello;
///
/// @end
///
/// // An interface can also conform to protocols
/// @interface Person <Protocol1, Protocol2>
///
/// @end
/// ```
#[derive(Debug, Clone)]
pub struct ObjCInterface {
    pub name: String,
    pub subclass: String,
    pub conforms_to: Vec<String>,
    pub category: Option<String>,

    pub instance_variables: Vec<ObjCPropertyField>,
    pub properties: Vec<ObjCPropertyField>,
    pub methods: Vec<ObjCMethod>,
}

/// An Objective-C @protocol
///
/// Contains the name of the protocol and its methods.
///
/// ```objc
/// @protocol Protocol
///
/// - (void)requiredMethod;
///
/// @optional
/// - (void)optionalMethod;
///
/// @end
/// ```
#[derive(Debug, Clone)]
pub struct ObjCProtocol {
    pub name: String,
    pub methods: Vec<ObjCMethod>,
    pub optional_methods: Vec<ObjCMethod>,
}

/// An Objective-C @autoreleasepool
///
/// Contains the body of statements to be executed within the autorelease pool.
///
/// ```objc
/// @autoreleasepool {
///     // Your code here
/// }
/// ```
#[derive(Debug, Clone)]
pub struct ObjCAutoreleasePool {
    pub body: Vec<ObjCStmt>,
}

/// An Objective-C @implementation
///
/// Contains the name of the implementation and its methods.
///
/// ```objc
/// @implementation Person
///
/// - (void)sayHello {
///     NSLog(@"Hello, world!");
/// }
///
/// @end
/// ```
#[derive(Debug, Clone)]
pub struct ObjCImplementation {
    pub name: String,
    pub methods: Vec<ObjCMethod>,
}

/// A top-level Objective-C item, such as an interface, protocol, implementation,
/// C struct, or C enum.
#[derive(Debug, Clone)]
pub enum ObjCItem {
    Interface(ObjCInterface),
    Protocol(ObjCProtocol),
    AutoreleasePool(ObjCAutoreleasePool),
    Implementation(ObjCImplementation),

    CStruct(ObjCCStruct),
    CEnum(ObjCCEnum),
}

#[derive(Debug, Clone)]
pub enum ObjCPrePostfixOp {
    Inc, // ++
    Dec, // --
}

#[derive(Debug, Clone)]
pub enum ObjCExpr {
    // Sentinel value, never emitted. Some expressions do not map 1:1 from Lilac to
    // Objective C because of the expression-based nature of Lilac.
    Sentinel,

    Lit(ast::Literal),
    Ident(String),

    Binary {
        op: ast::operator::BinaryOp,
        lhs: Box<ObjCExpr>,
        rhs: Box<ObjCExpr>,
    },

    Unary {
        op: ast::operator::UnaryOp,
        expr: Box<ObjCExpr>,
    },

    Prefix {
        op: ObjCPrePostfixOp,
        expr: Box<ObjCExpr>,
    },

    Postfix {
        op: ObjCPrePostfixOp,
        expr: Box<ObjCExpr>,
    },

    // C function call: NSLog(@"message")
    CCall {
        callee: Box<ObjCExpr>,
        arguments: Vec<ObjCExpr>,
    },

    // Objective-C style selector call: [target selectorName arg1:val]
    SelectorCall {
        target: Box<ObjCExpr>,
        selector: String,
        args: Vec<(String, ObjCExpr)>,
    },
}

#[derive(Debug, Clone)]
pub enum ObjCStmt {
    Expr(ObjCExpr),

    VarDecl {
        name: String,
        var_type: CType,
        initial_value: Option<ObjCExpr>,
    },

    Assign {
        target: Box<ObjCExpr>,
        value: Box<ObjCExpr>,
    },

    Return {
        value: Option<ObjCExpr>,
    },

    For {
        init: Box<ObjCExpr>,
        condition: Box<ObjCExpr>,
        increment: Box<ObjCExpr>,
        body: Vec<ObjCStmt>,
    },

    ForIn {
        var: String,
        iterable: Box<ObjCExpr>,
        body: Vec<ObjCStmt>,
    },

    While {
        condition: Box<ObjCExpr>,
        body: Vec<ObjCStmt>,
    },
}
