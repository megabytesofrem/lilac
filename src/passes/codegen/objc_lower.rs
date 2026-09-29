//! Objective-C code generation backend for the Lilac compiler
//!
//! NOTE: This is the most complete backend currently, as the C and JS backend would require re-implementing
//! objc_msgSend and retain/release functions.

use super::objc_ast::*;
use crate::ast;

pub struct LowerCtx {
    // Objective-C constructs collected during lowering
    protocols: Vec<ObjCProtocol>,
    interfaces: Vec<ObjCInterface>,
    implementations: Vec<ObjCImplementation>,

    // Expressions that still need to be lowered into Objective-C constructs
    worklist_to_lower: Vec<ast::Expr>,
}

impl LowerCtx {
    pub fn new() -> Self {
        Self {
            protocols: Vec::new(),
            interfaces: Vec::new(),
            implementations: Vec::new(),
            worklist_to_lower: Vec::new(),
        }
    }

    pub fn add_protocol(&mut self, protocol: ObjCProtocol) {
        self.protocols.push(protocol);
    }

    pub fn add_interface(&mut self, interface: ObjCInterface) {
        self.interfaces.push(interface);
    }

    pub fn add_implementation(&mut self, implementation: ObjCImplementation) {
        self.implementations.push(implementation);
    }

    // MARK: Lowering

    pub fn lower_expr(&mut self, expr: ast::Expr) -> ObjCExpr {
        match expr {
            ast::Expr::Lit(lit) => ObjCExpr::Lit(lit),
            ast::Expr::Ident(name) => ObjCExpr::Ident(name),

            ast::Expr::Binary { op, lhs, rhs } => ObjCExpr::Binary {
                op,
                lhs: Box::new(self.lower_expr(*lhs)),
                rhs: Box::new(self.lower_expr(*rhs)),
            },

            ast::Expr::Unary { op, expr } => ObjCExpr::Unary {
                op,
                expr: Box::new(self.lower_expr(*expr)),
            },

            ast::Expr::SelectorCall {
                target,
                selector,
                args,
            } => ObjCExpr::SelectorCall {
                target: Box::new(self.lower_expr(*target)),
                selector,
                args: args
                    .into_iter()
                    .map(|(name, expr)| (name, self.lower_expr(expr)))
                    .collect(),
            },

            ast::Expr::Let { .. } => {
                self.worklist_to_lower.push(expr);

                // Return a sentinel because we cannot directly lower let expressions to Objective-C
                ObjCExpr::Sentinel
            }

            ast::Expr::Assign { .. } => {
                self.worklist_to_lower.push(expr);

                // Return a sentinel because we cannot directly lower assign expressions to Objective-C
                ObjCExpr::Sentinel
            }

            ast::Expr::With { .. } => {
                self.worklist_to_lower.push(expr);

                // Return a sentinel because we cannot directly lower with expressions to Objective-C
                ObjCExpr::Sentinel
            }

            ast::Expr::For { .. } => {
                self.worklist_to_lower.push(expr);

                // Return a sentinel because we cannot directly lower for expressions to Objective-C
                ObjCExpr::Sentinel
            }

            ast::Expr::Until { .. } => {
                self.worklist_to_lower.push(expr);

                // Return a sentinel because we cannot directly lower until expressions to Objective-C
                ObjCExpr::Sentinel
            }

            ast::Expr::Block { .. } => {
                self.worklist_to_lower.push(expr);

                // Return a sentinel because we cannot directly lower block expressions to Objective-C
                ObjCExpr::Sentinel
            }

            // Add cases for different kinds of expressions here
            _ => unimplemented!("Lowering for this expression is not implemented yet"),
        }
    }

    pub fn lower_item(&mut self, item: ast::Item) -> ObjCItem {
        unimplemented!("Lowering for this item is not implemented yet")
    }
}
