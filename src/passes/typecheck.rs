use crate::ast::operator::{BinaryOp, UnaryOp};
use crate::ast::types::Type;
use crate::ast::{Expr, Item, Literal};
use crate::passes::typed_ast::{
    TypedExpr, TypedExprKind, TypedField, TypedItem, TypedMessage, TypedMessageHandler,
    TypedParameter,
};

/// Typechecker result type.
///
/// Represents either a successful type checking result, or a type error.
pub type TypeResult<T> = Result<T, TypeError>;

#[derive(Debug, Clone, PartialEq)]
pub enum TypeError {
    UnknownIdentifier(String),
    TypeMismatch {
        expected: Type,
        actual: Type,
    },
    ArityMismatch {
        expected: usize,
        actual: usize,
    },
    InvalidOperand {
        operator: String,
        operand: Type,
    },
    InvalidBinaryOperands {
        operator: BinaryOp,
        lhs: Type,
        rhs: Type,
    },
    NotCallable(Type),
    CannotInferEmptyCollection,
    Unsupported(&'static str),
}

#[derive(Debug, Clone, Default)]
pub struct TypeEnv {
    scopes: Vec<Vec<(String, Type)>>,
}

impl TypeEnv {
    pub fn new() -> Self {
        Self {
            scopes: vec![Vec::new()],
        }
    }

    pub fn define(&mut self, name: impl Into<String>, ty: Type) {
        if let Some(scope) = self.scopes.last_mut() {
            scope.push((name.into(), ty));
        }
    }

    pub fn lookup(&self, name: &str) -> Option<Type> {
        self.scopes
            .iter()
            .rev()
            .flat_map(|scope| scope.iter().rev())
            .find_map(|(bound_name, ty)| (bound_name == name).then(|| ty.clone()))
    }

    fn child(&self) -> Self {
        let mut child = self.clone();
        child.scopes.push(Vec::new());
        child
    }
}

#[derive(Debug, Clone, Default)]
pub struct TypeChecker {
    /// Environment used during type checking/inference.
    pub env: TypeEnv,
}

impl TypeChecker {
    pub fn new() -> Self {
        Self {
            env: TypeEnv::new(),
        }
    }

    pub fn infer_expr(&mut self, expr: &Expr) -> TypeResult<TypedExpr> {
        match expr {
            Expr::Lit(literal) => Ok(TypedExpr::new(
                TypedExprKind::Lit(literal.clone()),
                literal_type(literal),
            )),
            Expr::Ident(name) => self
                .env
                .lookup(name)
                .map(|ty| TypedExpr::new(TypedExprKind::Ident(name.clone()), ty))
                .ok_or_else(|| TypeError::UnknownIdentifier(name.clone())),
            Expr::Array(elements) => self.infer_array(elements),
            Expr::Tuple(elements) => {
                let typed = elements
                    .iter()
                    .map(|element| self.infer_expr(element))
                    .collect::<TypeResult<Vec<_>>>()?;
                let ty = Type::Tuple(typed.iter().map(|element| element.ty.clone()).collect());
                Ok(TypedExpr::new(TypedExprKind::Tuple(typed), ty))
            }
            Expr::Binary { op, lhs, rhs } => self.infer_binary(op.clone(), lhs, rhs),
            Expr::Unary { op, expr } => {
                let typed_expr = self.infer_expr(expr)?;
                let ty = infer_unary_type(op.clone(), &typed_expr.ty)?;
                Ok(TypedExpr::new(
                    TypedExprKind::Unary {
                        op: op.clone(),
                        expr: Box::new(typed_expr),
                    },
                    ty,
                ))
            }
            Expr::Call { callee, arguments } => self.infer_call(callee, arguments),
            Expr::Let {
                name,
                opt_type,
                value,
                body,
            } => {
                let typed_value = match opt_type {
                    Some(expected) => self.check_expr(value, expected)?,
                    None => self.infer_expr(value)?,
                };

                let mut body_checker = Self {
                    env: self.env.child(),
                };
                body_checker
                    .env
                    .define(name.clone(), typed_value.ty.clone());

                // Infer the type of the body expression within the new environment.
                let typed_body = body_checker.infer_expr(body)?;

                Ok(TypedExpr::new(
                    TypedExprKind::Let {
                        name: name.clone(),
                        declared_type: opt_type.clone(),
                        value: Box::new(typed_value),
                        body: Box::new(typed_body.clone()),
                    },
                    typed_body.ty,
                ))
            }
            Expr::Assign { target, value } => {
                let typed_target = self.infer_expr(target)?;
                let typed_value = self.check_expr(value, &typed_target.ty)?;
                Ok(TypedExpr::new(
                    TypedExprKind::Assign {
                        target: Box::new(typed_target),
                        value: Box::new(typed_value),
                    },
                    Type::Unit,
                ))
            }
            Expr::With { target, new_props } => {
                let typed_target = self.infer_expr(target)?;
                let typed_props = new_props
                    .iter()
                    .map(|(name, value)| Ok((name.clone(), self.infer_expr(value)?)))
                    .collect::<TypeResult<Vec<_>>>()?;
                Ok(TypedExpr::new(
                    TypedExprKind::With {
                        target: Box::new(typed_target.clone()),
                        new_props: typed_props,
                    },
                    typed_target.ty,
                ))
            }
            Expr::For {
                iterator,
                iterable,
                body,
            } => {
                let typed_iterable = self.infer_expr(iterable)?;
                let mut body_checker = Self {
                    env: self.env.child(),
                };
                body_checker
                    .env
                    .define(iterator.clone(), Type::Named("IteratorItem".into()));

                // Infer the type of the body expression within the new environment.
                let typed_body = body_checker.infer_expr(body)?;

                Ok(TypedExpr::new(
                    TypedExprKind::For {
                        iterator: iterator.clone(),
                        iterable: Box::new(typed_iterable),
                        body: Box::new(typed_body),
                    },
                    Type::Unit,
                ))
            }
            Expr::Until { condition, body } => {
                let typed_condition = self.check_expr(condition, &Type::Bool)?;
                let typed_body = self.infer_expr(body)?;
                Ok(TypedExpr::new(
                    TypedExprKind::Until {
                        condition: Box::new(typed_condition),
                        body: Box::new(typed_body),
                    },
                    Type::Unit,
                ))
            }
            Expr::Block(expressions) => {
                let typed = expressions
                    .iter()
                    .map(|expression| self.infer_expr(expression))
                    .collect::<TypeResult<Vec<_>>>()?;
                let ty = typed
                    .last()
                    .map(|expression| expression.ty.clone())
                    .unwrap_or(Type::Unit);
                Ok(TypedExpr::new(TypedExprKind::Block(typed), ty))
            }

            // TODO: inference for member and protocol lookup
            Expr::Member(_, _) | Expr::SelectorCall { .. } => {
                Err(TypeError::Unsupported("member and protocol lookup"))
            }
        }
    }

    pub fn check_expr(&mut self, expr: &Expr, expected: &Type) -> TypeResult<TypedExpr> {
        let typed = self.infer_expr(expr)?;

        // Check if the inferred type matches the expected type
        if &typed.ty == expected {
            Ok(typed)
        } else {
            Err(TypeError::TypeMismatch {
                expected: expected.clone(),
                actual: typed.ty,
            })
        }
    }

    pub fn check_item(&mut self, item: &Item) -> TypeResult<TypedItem> {
        match item {
            Item::Expr(expr) => Ok(TypedItem::Expr(self.infer_expr(expr)?)),
            Item::StructDef { name, fields } => Ok(TypedItem::StructDef {
                name: name.clone(),
                fields: fields
                    .iter()
                    .map(|(name, ty)| TypedField {
                        name: name.clone(),
                        ty: ty.clone(),
                    })
                    .collect(),
            }),
            Item::EnumDef { name, variants } => Ok(TypedItem::EnumDef {
                name: name.clone(),
                variants: variants.clone(),
            }),
            Item::FunctionDef {
                name,
                params,
                visibility,
                return_type,
                body,
            } => {
                let mut function_checker = Self {
                    env: self.env.child(),
                };
                let typed_params = params
                    .iter()
                    .map(|(name, ty)| {
                        function_checker.env.define(name.clone(), ty.clone());
                        TypedParameter {
                            name: name.clone(),
                            ty: ty.clone(),
                        }
                    })
                    .collect();

                // Check the body of the function with the expected return type.
                let typed_body = function_checker.check_expr(body, return_type)?;

                self.env.define(
                    name.clone(),
                    Type::Function {
                        parameters: params.iter().map(|(_, ty)| ty.clone()).collect(),
                        return_type: Box::new(return_type.clone()),
                    },
                );
                Ok(TypedItem::FunctionDef {
                    name: name.clone(),
                    params: typed_params,
                    visibility: visibility.clone(),
                    return_type: return_type.clone(),
                    body: typed_body,
                })
            }
            Item::ProtocolDef { name, messages } => Ok(TypedItem::ProtocolDef {
                name: name.clone(),
                messages: messages
                    .iter()
                    .map(|message| TypedMessage {
                        target: message.target.clone(),
                        selector: message.selector.0.clone(),
                        keyword_args: message
                            .keyword_args
                            .iter()
                            .map(|(name, ty)| TypedParameter {
                                name: name.clone(),
                                ty: ty.clone(),
                            })
                            .collect(),
                        return_type: message.return_type.clone(),
                    })
                    .collect(),
            }),
            Item::ProtocolImpl {
                name,
                on_target,
                message_handlers,
            } => {
                // Collect and type check all message handlers
                let handlers = message_handlers
                    .iter()
                    .map(|handler| {
                        Ok(TypedMessageHandler {
                            message: TypedMessage {
                                target: handler.message.target.clone(),
                                selector: handler.message.selector.0.clone(),
                                keyword_args: handler
                                    .message
                                    .keyword_args
                                    .iter()
                                    .map(|(name, ty)| TypedParameter {
                                        name: name.clone(),
                                        ty: ty.clone(),
                                    })
                                    .collect(),
                                return_type: handler.message.return_type.clone(),
                            },
                            handler_body: self.infer_expr(&handler.handler_body)?,
                        })
                    })
                    .collect::<TypeResult<Vec<_>>>()?;
                Ok(TypedItem::ProtocolImpl {
                    name: name.clone(),
                    on_target: on_target.clone(),
                    message_handlers: handlers,
                })
            }
        }
    }

    // MARK: Expression Inference

    fn infer_array(&mut self, elements: &[Expr]) -> TypeResult<TypedExpr> {
        let first = elements
            .first()
            .ok_or(TypeError::CannotInferEmptyCollection)?;
        let first_typed = self.infer_expr(first)?;
        let element_type = first_typed.ty.clone();
        let mut typed = vec![first_typed];

        // Type check the remaining elements against the type of the first element.
        for element in &elements[1..] {
            typed.push(self.check_expr(element, &element_type)?);
        }

        Ok(TypedExpr::new(
            TypedExprKind::Array(typed),
            Type::Array(Box::new(element_type)),
        ))
    }

    fn infer_binary(&mut self, op: BinaryOp, lhs: &Expr, rhs: &Expr) -> TypeResult<TypedExpr> {
        let typed_lhs = self.infer_expr(lhs)?;
        let typed_rhs = self.infer_expr(rhs)?;

        // Determine the result type of the binary operation based on typed_lhs and rhs.
        let result_type = binary_type(op.clone(), &typed_lhs.ty, &typed_rhs.ty)?;

        Ok(TypedExpr::new(
            TypedExprKind::Binary {
                op,
                lhs: Box::new(typed_lhs),
                rhs: Box::new(typed_rhs),
            },
            result_type,
        ))
    }

    fn infer_call(&mut self, callee: &Expr, arguments: &[Expr]) -> TypeResult<TypedExpr> {
        let typed_callee = self.infer_expr(callee)?;

        // Not a callable type: the callee must be a function
        let Type::Function {
            parameters,
            return_type,
        } = typed_callee.ty.clone()
        else {
            return Err(TypeError::NotCallable(typed_callee.ty));
        };

        // Arity mismatch: argument count does not match
        if parameters.len() != arguments.len() {
            return Err(TypeError::ArityMismatch {
                expected: parameters.len(),
                actual: arguments.len(),
            });
        }

        // Type check each argument against the corresponding parameter type.
        let typed_arguments = arguments
            .iter()
            .zip(&parameters)
            .map(|(argument, parameter)| self.check_expr(argument, parameter))
            .collect::<TypeResult<Vec<_>>>()?;

        Ok(TypedExpr::new(
            TypedExprKind::Call {
                callee: Box::new(typed_callee),
                arguments: typed_arguments,
            },
            *return_type,
        ))
    }
}

// MARK: Helper Functions

fn literal_type(literal: &Literal) -> Type {
    match literal {
        Literal::Int(_) => Type::I64,
        Literal::Float(_) => Type::F64,
        Literal::Str(_) => Type::Str,
        Literal::Char(_) => Type::Char,
        Literal::Bool(_) => Type::Bool,
    }
}

fn infer_unary_type(op: UnaryOp, operand: &Type) -> TypeResult<Type> {
    match op {
        UnaryOp::Neg if operand.is_numeric() => Ok(operand.clone()),
        UnaryOp::Not if *operand == Type::Bool => Ok(Type::Bool),
        _ => Err(TypeError::InvalidOperand {
            operator: format!("{op}"),
            operand: operand.clone(),
        }),
    }
}

fn binary_type(op: BinaryOp, lhs: &Type, rhs: &Type) -> TypeResult<Type> {
    match op {
        BinaryOp::Add | BinaryOp::Sub | BinaryOp::Mul | BinaryOp::Div
            if lhs == rhs && lhs.is_numeric() =>
        {
            Ok(lhs.clone())
        }
        BinaryOp::Eq | BinaryOp::Ne | BinaryOp::Lt | BinaryOp::Gt | BinaryOp::Le | BinaryOp::Ge
            if lhs == rhs =>
        {
            Ok(Type::Bool)
        }
        _ => Err(TypeError::InvalidBinaryOperands {
            operator: op,
            lhs: lhs.clone(),
            rhs: rhs.clone(),
        }),
    }
}
