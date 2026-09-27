use crate::ast;
use std::fmt;

pub struct CodegenCtx {
    emitted_code: String,
}

#[derive(Debug, Clone)]
#[allow(dead_code)]
pub enum CType {
    Int8,
    Int16,
    Int32,
    Int64,
    Float,
    Double,
    Bool,
    Char,
    String,
    Void,

    // Named type
    Named(String),

    // NS-prefixed types e.g NSString, NSNumber
    NS(String),

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
        }
    }
}

// MARK: Objective C AST

/// A tag for an Objective-C property attribute.
#[derive(Debug, Clone)]
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
pub struct ObjCProperty {
    name: String,
    attribute_tags: Vec<AttributeTag>,
    ctype: CType,
}

/// An Objective-C method
///
/// NOTE: If the method is static, it will be prefixed with a `+` rather than a `-`.
#[derive(Debug, Clone)]
pub struct ObjCMethod {
    name: String,
    parameters: Vec<(String, CType)>,
    return_type: CType,
    is_static_method: bool,
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
    name: String,
    subclass: String,
    conforms_to: Vec<String>,
    category: Option<String>,

    instance_variables: Vec<ObjCProperty>,
    properties: Vec<ObjCProperty>,
    methods: Vec<ObjCMethod>,
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
    name: String,
    methods: Vec<ObjCMethod>,
    optional_methods: Vec<ObjCMethod>,
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
    body: Vec<ObjCStmt>,
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
    name: String,
    methods: Vec<ObjCMethod>,
}

#[derive(Debug, Clone)]
pub enum ObjCPrePostfixOp {
    Inc, // ++
    Dec, // --
}

#[derive(Debug, Clone)]
pub enum ObjCExpr {
    Literal(ast::Literal),
    Ident(String),

    Binary {
        op: ast::BinaryOp,
        left: Box<ObjCExpr>,
        right: Box<ObjCExpr>,
    },

    Unary {
        op: ast::UnaryOp,
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
        value: Box<ObjCExpr>,
    },

    For {
        init: Box<ObjCStmt>,
        condition: Box<ObjCExpr>,
        increment: Box<ObjCStmt>,
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

// MARK: Objective-C code emission

impl CodegenCtx {
    pub fn new() -> Self {
        Self {
            emitted_code: String::new(),
        }
    }

    fn format_pre_postfix_op(op: &ObjCPrePostfixOp) -> &'static str {
        match op {
            ObjCPrePostfixOp::Inc => "++",
            ObjCPrePostfixOp::Dec => "--",
        }
    }

    fn format_method_signature(&self, method: &ObjCMethod) -> String {
        let prefix = if method.is_static_method { "+" } else { "-" };

        // Handle zero-parameter methods: e.g., - (void)sayHello;
        if method.parameters.is_empty() {
            return format!("{} ({}){};", prefix, method.return_type, method.name);
        }

        let mut sig = format!("{} ({}){}", prefix, method.return_type, method.name);

        // Format multi-keyword Objective-C parameters:
        // [name]:(type)paramName keyword2:(type)paramName2
        for (i, (keyword_or_name, ctype)) in method.parameters.iter().enumerate() {
            if i == 0 {
                // The first parameter's colon attaches directly to the primary selector name
                sig.push_str(&format!(":({}){}", ctype, keyword_or_name));
            } else {
                // Subsequent parameters carry their own keyword label segment preceding the colon
                sig.push_str(&format!(
                    " {}:({}){}",
                    keyword_or_name, ctype, keyword_or_name
                ));
            }
        }

        sig.push(';');
        sig
    }

    pub fn emit_property(&mut self, property: ObjCProperty) -> String {
        let attribute = property
            .attribute_tags
            .iter()
            .map(|attr| attr.to_string())
            .collect::<Vec<_>>()
            .join(", ");

        let emitted = format!(
            "@property ({}) {} {};\n",
            attribute, property.ctype, property.name
        );

        emitted
    }

    pub fn emit_autorelease_pool(&mut self, pool: ObjCAutoreleasePool) {
        self.emitted_code.push_str("@autoreleasepool {\n");
        for _stmt in pool.body {
            // Here you would recursively emit each statement
            // For simplicity, we'll just use a placeholder
            self.emitted_code.push_str("    // Statement\n");
        }
        self.emitted_code.push_str("}\n");
    }

    pub fn emit_protocol(&mut self, protocol: ObjCProtocol) {
        self.emitted_code
            .push_str(&format!("@protocol {}\n", protocol.name));

        for method in &protocol.methods {
            let formatted_sig = self.format_method_signature(method);
            self.emitted_code.push_str(&format!("{}\n", formatted_sig));
        }

        if !protocol.optional_methods.is_empty() {
            self.emitted_code.push_str("// MARK: Optional methods\n");
        }

        for method in &protocol.optional_methods {
            self.emitted_code.push_str("@optional\n");
            let formatted_sig = self.format_method_signature(method);
            self.emitted_code.push_str(&format!("{}\n", formatted_sig));
        }

        self.emitted_code.push_str(&format!("@end\n"));
    }

    pub fn emit_interface(&mut self, interface: ObjCInterface) {
        // Handle protocol conformance if protocols are present: @interface Person : NSObject <Protocol1, Protocol2>
        let conforms_str = if interface.conforms_to.is_empty() {
            String::new()
        } else {
            format!(" <{}>", interface.conforms_to.join(", "))
        };

        // Handle optional category extensions: @interface Person (MyCategory)
        if let Some(category) = &interface.category {
            self.emitted_code.push_str(&format!(
                "@interface {}({}){}\n",
                interface.name, category, conforms_str
            ));
        } else {
            self.emitted_code.push_str(&format!(
                "@interface {} : {}{}\n",
                interface.name, interface.subclass, conforms_str
            ));
        }

        // Instance variables inside braces: { int32_t counter; }
        if !interface.instance_variables.is_empty() {
            self.emitted_code.push_str("{\n");
            for field in &interface.instance_variables {
                self.emitted_code
                    .push_str(&format!("    {} {};\n", field.ctype, field.name));
            }
            self.emitted_code.push_str("}\n");
        }

        // Properties: @property (nonatomic, strong) NSString *name;
        for property in &interface.properties {
            let property_str = self.emit_property(property.clone());
            self.emitted_code.push_str(&property_str);
        }

        // Methods: - (int32_t)sum:(int32_t)firstNumber secondNumber:(int32_t)secondNumber;
        for method in &interface.methods {
            let formatted_sig = self.format_method_signature(method);
            self.emitted_code.push_str(&format!("{}\n", formatted_sig));
        }

        self.emitted_code.push_str("@end\n");
    }

    pub fn emit_implementation(&mut self, implementation: ObjCImplementation) {
        self.emitted_code
            .push_str(&format!("@implementation {}\n", implementation.name));

        // Methods
        for _method in implementation.methods {
            // Here you would recursively emit each method
            // For simplicity, we'll just use a placeholder
            self.emitted_code.push_str("    // Method\n");
        }
        self.emitted_code.push_str("@end\n");
    }

    pub fn emit_literal(&self, lit: ast::Literal) -> String {
        match lit {
            ast::Literal::Int(val) => format!("{}", val),
            ast::Literal::Float(val) => format!("{}", val),
            ast::Literal::Str(val) => format!("@\"{}\"", val),
            ast::Literal::Char(val) => format!("'{}'", val),

            ast::Literal::Bool(val) => match val {
                true => format!("YES"),
                false => format!("NO"),
            },
        }
    }

    fn emit_inner_expr(&self, expr: ObjCExpr) -> String {
        match expr {
            ObjCExpr::Literal(lit) => self.emit_literal(lit),
            ObjCExpr::Ident(name) => name,
            ObjCExpr::Binary { op, left, right } => format!(
                "({} {} {})",
                self.emit_inner_expr(*left),
                op,
                self.emit_inner_expr(*right)
            ),
            ObjCExpr::Unary { op, expr } => {
                format!("({}{})", op, self.emit_inner_expr(*expr))
            }
            ObjCExpr::Prefix { op, expr } => {
                format!(
                    "{}{}",
                    Self::format_pre_postfix_op(&op),
                    self.emit_inner_expr(*expr)
                )
            }
            ObjCExpr::Postfix { op, expr } => {
                format!(
                    "{}{}",
                    self.emit_inner_expr(*expr),
                    Self::format_pre_postfix_op(&op)
                )
            }
            ObjCExpr::CCall { callee, arguments } => format!(
                "{}({})",
                self.emit_inner_expr(*callee),
                arguments
                    .into_iter()
                    .map(|argument| self.emit_inner_expr(argument))
                    .collect::<Vec<_>>()
                    .join(", ")
            ),
            ObjCExpr::SelectorCall {
                target,
                selector,
                args,
            } => {
                let mut output = format!("[{} {}", self.emit_inner_expr(*target), selector);
                for (name, argument) in args {
                    output.push_str(&format!(" {}:{}", name, self.emit_inner_expr(argument)));
                }
                output.push(']');
                output
            }
        }
    }

    pub fn emit_expr(&mut self, expr: ObjCExpr) {
        self.emitted_code.push_str(&self.emit_inner_expr(expr));
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn emits_property() {
        let mut codegen = CodegenCtx::new();

        let property = ObjCProperty {
            name: "name".to_string(),
            ctype: CType::String,
            attribute_tags: vec![AttributeTag::Nonatomic, AttributeTag::Strong],
        };

        let emitted = codegen.emit_property(property);
        println!("\n\n{}\n\n", emitted);

        assert!(emitted.contains("@property (nonatomic, strong) NSString * name;"));
    }

    #[test]
    fn emits_protocol() {
        let mut codegen = CodegenCtx::new();

        let methods = vec![
            ObjCMethod {
                name: "calculateArea".to_string(),
                parameters: vec![],
                return_type: CType::Float,
                is_static_method: false,
            },
            ObjCMethod {
                name: "calculatePerimeter".to_string(),
                parameters: vec![],
                return_type: CType::Float,
                is_static_method: false,
            },
        ];

        let protocol = ObjCProtocol {
            name: "Shape".to_string(),
            methods: methods,
            optional_methods: vec![],
        };
        codegen.emit_protocol(protocol);
        let emitted = codegen.emitted_code;

        println!("\n\n{}\n\n", emitted);

        assert!(emitted.contains("@protocol Shape"));
        assert!(emitted.contains("@end"));
    }

    #[test]
    fn emits_interface() {
        let mut codegen = CodegenCtx::new();

        // let methods = vec![ObjCMethod {
        //     name: "sum".to_string(),
        //     parameters: vec![
        //         ("firstNumber".to_string(), CType::Int32),
        //         ("secondNumber".to_string(), CType::Int32),
        //     ],
        //     return_type: CType::Int32,
        //     is_static_method: false,
        // }];

        let properties = vec![
            ObjCProperty {
                name: "width".to_string(),
                ctype: CType::Float,
                attribute_tags: vec![AttributeTag::Nonatomic, AttributeTag::Assign],
            },
            ObjCProperty {
                name: "height".to_string(),
                ctype: CType::Float,
                attribute_tags: vec![AttributeTag::Nonatomic, AttributeTag::Assign],
            },
        ];

        let interface = ObjCInterface {
            name: "Rectangle".to_string(),
            subclass: "NSObject".to_string(),
            category: None,
            instance_variables: vec![],
            conforms_to: vec!["Shape".to_string()],
            properties: properties,
            methods: vec![],
        };
        codegen.emit_interface(interface);
        let emitted = codegen.emitted_code;

        println!("\n\n{}\n\n", emitted);

        assert!(emitted.contains("@interface Rectangle"));
        assert!(emitted.contains("@end"));
    }

    #[test]
    fn emits_expressions() {
        let mut codegen = CodegenCtx::new();
        codegen.emit_expr(ObjCExpr::Binary {
            op: ast::BinaryOp::Add,
            left: Box::new(ObjCExpr::Literal(ast::Literal::Int(1))),
            right: Box::new(ObjCExpr::Binary {
                op: ast::BinaryOp::Mul,
                left: Box::new(ObjCExpr::Literal(ast::Literal::Int(2))),
                right: Box::new(ObjCExpr::Literal(ast::Literal::Int(3))),
            }),
        });
        assert_eq!(codegen.emitted_code, "(1 + (2 * 3))");
    }

    #[test]
    fn emits_calls_and_selector_calls() {
        let mut codegen = CodegenCtx::new();
        codegen.emit_expr(ObjCExpr::CCall {
            callee: Box::new(ObjCExpr::Ident("max".into())),
            arguments: vec![
                ObjCExpr::Literal(ast::Literal::Int(1)),
                ObjCExpr::Literal(ast::Literal::Int(2)),
            ],
        });
        assert_eq!(codegen.emitted_code, "max(1, 2)");

        codegen.emitted_code.clear();
        codegen.emit_expr(ObjCExpr::SelectorCall {
            target: Box::new(ObjCExpr::Ident("window".into())),
            selector: "resize".into(),
            args: vec![("width".into(), ObjCExpr::Literal(ast::Literal::Int(640)))],
        });
        assert_eq!(codegen.emitted_code, "[window resize width:640]");
    }

    #[test]
    fn emits_prefix_and_postfix_operators() {
        let mut codegen = CodegenCtx::new();
        codegen.emit_expr(ObjCExpr::Prefix {
            op: ObjCPrePostfixOp::Inc,
            expr: Box::new(ObjCExpr::Ident("counter".into())),
        });
        codegen.emitted_code.push(' ');
        codegen.emit_expr(ObjCExpr::Postfix {
            op: ObjCPrePostfixOp::Dec,
            expr: Box::new(ObjCExpr::Ident("counter".into())),
        });
        assert_eq!(codegen.emitted_code, "++counter counter--");
    }
}
