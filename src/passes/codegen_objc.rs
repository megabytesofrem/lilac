//! Objective-C code generation backend for the Lilac compiler
//!
//! NOTE: This is the most complete backend currently, as the C and JS backend would require re-implementing
//! objc_msgSend and retain/release functions.

use crate::{ast, passes::objc_ast::*};

/// Context for Objective-C code generation
pub struct CodegenCtx {
    emitted_code: String,
    indentation_level: usize,
}

// MARK: Objective-C code emission

impl CodegenCtx {
    pub fn new() -> Self {
        Self {
            emitted_code: String::new(),
            indentation_level: 0,
        }
    }

    fn format_pre_postfix_op(op: &ObjCPrePostfixOp) -> &'static str {
        match op {
            ObjCPrePostfixOp::Inc => "++",
            ObjCPrePostfixOp::Dec => "--",
        }
    }

    fn format_method_signature(&self, method: &ObjCMethod) -> String {
        let prefix = if method.prototype.is_static_method {
            "+"
        } else {
            "-"
        };

        // Handle zero-parameter methods: e.g., - (void)sayHello;
        if method.prototype.parameters.is_empty() {
            return format!(
                "{} ({}){};",
                prefix, method.prototype.return_type, method.prototype.name
            );
        }

        let mut sig = format!(
            "{} ({}){}",
            prefix, method.prototype.return_type, method.prototype.name
        );

        // Format multi-keyword Objective-C parameters:
        // [name]:(type)paramName keyword2:(type)paramName2
        for (i, (keyword_or_name, ctype)) in method.prototype.parameters.iter().enumerate() {
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

    fn emit_indent(&mut self) {
        self.emitted_code
            .push_str(&"    ".repeat(self.indentation_level));
    }

    fn emit_body(&mut self, body: Vec<ObjCStmt>) {
        self.indentation_level += 1;
        for stmt in body {
            self.emit_stmt(stmt);
        }
        self.indentation_level -= 1;
    }

    pub fn emit_property(&mut self, property: ObjCPropertyField) -> String {
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
                "@interface {} ({}){}\n",
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
        for method in implementation.methods {
            self.emit_method_body(method);
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
            ObjCExpr::Sentinel => String::new(),

            ObjCExpr::Lit(lit) => self.emit_literal(lit),
            ObjCExpr::Ident(name) => name,
            ObjCExpr::Binary { op, lhs, rhs } => format!(
                "({} {} {})",
                self.emit_inner_expr(*lhs),
                op,
                self.emit_inner_expr(*rhs)
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

    pub fn emit_stmt(&mut self, stmt: ObjCStmt) {
        match stmt {
            ObjCStmt::Expr(expr) => {
                self.emit_indent();
                self.emit_expr(expr);
                self.emitted_code.push_str(";\n");
            }

            ObjCStmt::VarDecl {
                name,
                var_type,
                initial_value,
            } => {
                self.emit_indent();
                if let Some(initial_value) = initial_value {
                    self.emitted_code.push_str(&format!(
                        "{} {} = {};\n",
                        self.emit_inner_expr(ObjCExpr::Ident(var_type.to_string())),
                        name,
                        self.emit_inner_expr(initial_value)
                    ));
                } else {
                    self.emitted_code.push_str(&format!(
                        "{} {};\n",
                        self.emit_inner_expr(ObjCExpr::Ident(var_type.to_string())),
                        name,
                    ));
                }
            }

            ObjCStmt::Assign { target, value } => {
                self.emit_indent();
                self.emitted_code.push_str(&format!(
                    "{} = {};\n",
                    self.emit_inner_expr(*target),
                    self.emit_inner_expr(*value)
                ));
            }

            ObjCStmt::Return { value } => {
                self.emit_indent();

                if let Some(value) = value {
                    self.emitted_code
                        .push_str(&format!("return {};\n", self.emit_inner_expr(value)));
                } else {
                    self.emitted_code.push_str("return;\n");
                }
            }

            ObjCStmt::For {
                init,
                condition,
                increment,
                body,
            } => {
                self.emit_indent();
                self.emitted_code.push_str(&format!(
                    "for ({}; {}; {}) {{\n",
                    self.emit_inner_expr(*init),
                    self.emit_inner_expr(*condition),
                    self.emit_inner_expr(*increment)
                ));
                self.emit_body(body);
                self.emit_indent();
                self.emitted_code.push_str("}\n");
            }

            ObjCStmt::ForIn {
                var,
                iterable,
                body,
            } => {
                self.emit_indent();
                self.emitted_code.push_str(&format!(
                    "for (id {} in {}) {{\n",
                    var,
                    self.emit_inner_expr(*iterable)
                ));
                self.emit_body(body);
                self.emit_indent();
                self.emitted_code.push_str("}\n");
            }

            ObjCStmt::While { condition, body } => {
                self.emit_indent();
                self.emitted_code.push_str(&format!(
                    "while ({}) {{\n",
                    self.emit_inner_expr(*condition)
                ));
                self.emit_body(body);
                self.emit_indent();
                self.emitted_code.push_str("}\n");
            }
        }
    }

    pub fn emit_method_body(&mut self, method: ObjCMethod) {
        self.emit_indent();
        if method.prototype.is_static_method {
            self.emitted_code.push_str(&format!(
                "+ ({}){}",
                method.prototype.return_type, method.prototype.name
            ));
        } else {
            self.emitted_code.push_str(&format!(
                "- ({}){}",
                method.prototype.return_type, method.prototype.name
            ));
        }
        if !method.prototype.parameters.is_empty() {
            let params: Vec<String> = method
                .prototype
                .parameters
                .into_iter()
                .map(|p| format!("{}:({})", p.0, p.1))
                .collect();
            self.emitted_code.push_str(&params.join(" "));
        }
        self.emitted_code.push_str(" {\n");
        self.emit_body(method.body);
        self.emit_indent();
        self.emitted_code.push_str("}\n");
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn emits_property() {
        let mut codegen = CodegenCtx::new();

        let property = ObjCPropertyField {
            name: "name".to_string(),
            ctype: CType::String,
            attribute_tags: vec![AttributeTag::Nonatomic, AttributeTag::Strong],
        };

        let emitted = codegen.emit_property(property);

        assert!(emitted.contains("@property (nonatomic, strong) NSString * name;"));
    }

    #[test]
    fn emits_protocol() {
        let mut codegen = CodegenCtx::new();

        let methods = vec![
            ObjCMethod {
                prototype: ObjCPrototype {
                    name: "calculateArea".to_string(),
                    parameters: vec![],
                    return_type: CType::Float,
                    is_static_method: false,
                },
                body: vec![],
            },
            ObjCMethod {
                prototype: ObjCPrototype {
                    name: "calculatePerimeter".to_string(),
                    parameters: vec![],
                    return_type: CType::Float,
                    is_static_method: false,
                },
                body: vec![],
            },
        ];

        let protocol = ObjCProtocol {
            name: "Shape".to_string(),
            methods: methods,
            optional_methods: vec![],
        };
        codegen.emit_protocol(protocol);
        let emitted = codegen.emitted_code;

        println!("\n\nEmitted protocol:\n\n{}\n\n", emitted);

        assert!(emitted.contains("@protocol Shape"));
        assert!(emitted.contains("@end"));
    }

    #[test]
    fn emits_interface() {
        let mut codegen = CodegenCtx::new();

        let properties = vec![
            ObjCPropertyField {
                name: "width".to_string(),
                ctype: CType::Float,
                attribute_tags: vec![AttributeTag::Nonatomic, AttributeTag::Assign],
            },
            ObjCPropertyField {
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

        println!("\n\nEmitted interface:\n\n{}\n\n", emitted);

        assert!(emitted.contains("@interface Rectangle"));
        assert!(emitted.contains("@end"));
    }

    #[test]
    fn emits_method_body() {
        let mut codegen = CodegenCtx::new();
        let method = ObjCMethod {
            prototype: ObjCPrototype {
                name: "calculateArea".to_string(),
                parameters: vec![],
                return_type: CType::Float,
                is_static_method: false,
            },
            body: vec![ObjCStmt::Return {
                value: Some(ObjCExpr::Binary {
                    op: ast::BinaryOp::Mul,
                    lhs: Box::new(ObjCExpr::Ident("width".into())),
                    rhs: Box::new(ObjCExpr::Ident("height".into())),
                }),
            }],
        };
        codegen.emit_method_body(method);
        let emitted = codegen.emitted_code;

        println!("\n\nEmitted method body:\n\n{}\n\n", emitted);

        assert!(emitted.contains("- (float)calculateArea"));
        assert!(emitted.contains("{"));
        assert!(emitted.contains("}"));
    }

    #[test]
    fn emits_implementation() {
        let mut codegen = CodegenCtx::new();
        let method = ObjCMethod {
            prototype: ObjCPrototype {
                name: "calculateArea".to_string(),
                parameters: vec![],
                return_type: CType::Float,
                is_static_method: false,
            },
            body: vec![ObjCStmt::Return {
                value: Some(ObjCExpr::Binary {
                    op: ast::BinaryOp::Mul,
                    lhs: Box::new(ObjCExpr::Ident("width".into())),
                    rhs: Box::new(ObjCExpr::Ident("height".into())),
                }),
            }],
        };

        let implementation = ObjCImplementation {
            name: "Rectangle".to_string(),
            methods: vec![method],
        };
        codegen.emit_implementation(implementation);
        let emitted = codegen.emitted_code;

        println!("\n\nEmitted implementation:\n\n{}\n\n", emitted);

        assert!(emitted.contains("@implementation Rectangle"));
        assert!(emitted.contains("@end"));
    }

    #[test]
    fn emits_expressions() {
        let mut codegen = CodegenCtx::new();
        codegen.emit_expr(ObjCExpr::Binary {
            op: ast::BinaryOp::Add,
            lhs: Box::new(ObjCExpr::Lit(ast::Literal::Int(1))),
            rhs: Box::new(ObjCExpr::Binary {
                op: ast::BinaryOp::Mul,
                lhs: Box::new(ObjCExpr::Lit(ast::Literal::Int(2))),
                rhs: Box::new(ObjCExpr::Lit(ast::Literal::Int(3))),
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
                ObjCExpr::Lit(ast::Literal::Int(1)),
                ObjCExpr::Lit(ast::Literal::Int(2)),
            ],
        });
        assert_eq!(codegen.emitted_code, "max(1, 2)");

        codegen.emitted_code.clear();
        codegen.emit_expr(ObjCExpr::SelectorCall {
            target: Box::new(ObjCExpr::Ident("window".into())),
            selector: "resize".into(),
            args: vec![("width".into(), ObjCExpr::Lit(ast::Literal::Int(640)))],
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

    #[test]
    fn emits_loops_with_stateful_indentation() {
        let mut codegen = CodegenCtx::new();
        codegen.emit_stmt(ObjCStmt::ForIn {
            var: "item".into(),
            iterable: Box::new(ObjCExpr::Ident("items".into())),
            body: vec![ObjCStmt::While {
                condition: Box::new(ObjCExpr::Ident("running".into())),
                body: vec![ObjCStmt::Return {
                    value: Some(ObjCExpr::Ident("item".into())),
                }],
            }],
        });

        assert_eq!(
            codegen.emitted_code,
            "for (id item in items) {\n    while (running) {\n        return item;\n    }\n}\n"
        );
    }

    #[test]
    fn emits_c_style_for_loop() {
        let mut codegen = CodegenCtx::new();
        codegen.emit_stmt(ObjCStmt::For {
            init: Box::new(ObjCExpr::Ident("i = 0".into())),
            condition: Box::new(ObjCExpr::Ident("i < 3".into())),
            increment: Box::new(ObjCExpr::Postfix {
                op: ObjCPrePostfixOp::Inc,
                expr: Box::new(ObjCExpr::Ident("i".into())),
            }),
            body: vec![ObjCStmt::Expr(ObjCExpr::CCall {
                callee: Box::new(ObjCExpr::Ident("work".into())),
                arguments: vec![],
            })],
        });

        assert_eq!(
            codegen.emitted_code,
            "for (i = 0; i < 3; i++) {\n    work();\n}\n"
        );
    }
}
