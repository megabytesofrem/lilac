use crate::ast::operator::{BinaryOp, UnaryOp};
use crate::ast::types::Type;
use crate::ast::{DoBlock, Expr, Item, Literal};
use crate::passes::typed_ast::{
    TypedExpr, TypedExprKind, TypedField, TypedItem, TypedLetBinding, TypedMessage,
    TypedMessageHandler, TypedParameter,
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
            Expr::Lambda { .. } => Err(TypeError::Unsupported(
                "lambda inference requires an expected function type",
            )),
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
            Expr::DoBlock(block) => self.infer_do_block(block),
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
        if let Expr::Lambda { parameter, body } = expr {
            let Type::Function {
                parameters,
                return_type,
            } = expected
            else {
                return Err(TypeError::TypeMismatch {
                    expected: expected.clone(),
                    actual: Type::Named("lambda".into()),
                });
            };
            if parameters.len() != 1 {
                return Err(TypeError::ArityMismatch {
                    expected: 1,
                    actual: parameters.len(),
                });
            }
            let mut lambda_checker = Self {
                env: self.env.child(),
            };
            lambda_checker
                .env
                .define(parameter.clone(), parameters[0].clone());
            let typed_body = lambda_checker.check_expr(body, return_type)?;
            return Ok(TypedExpr::new(
                TypedExprKind::Lambda {
                    parameter: parameter.clone(),
                    body: Box::new(typed_body),
                },
                expected.clone(),
            ));
        }

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
            Item::ClassDef {
                name,
                superclass,
                conforms,
                fields,
                methods,
                override_methods,
            } => Ok(TypedItem::ClassDef {
                name: name.clone(),
                superclass: superclass.clone(),
                conforms: conforms.clone(),
                fields: fields
                    .iter()
                    .map(|(name, ty)| TypedField {
                        name: name.clone(),
                        ty: ty.clone(),
                    })
                    .collect(),
                // TODO: handle typechecking for methods and override_methods
                methods: Vec::new(),
                override_methods: Vec::new(),
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

    fn infer_do_block(&mut self, block: &DoBlock) -> TypeResult<TypedExpr> {
        let mut block_checker = Self {
            env: self.env.child(),
        };
        let mut bindings = Vec::with_capacity(block.bindings.len());

        for binding in &block.bindings {
            let value = match &binding.opt_type {
                Some(expected) => block_checker.check_expr(&binding.value, expected)?,
                None => block_checker.infer_expr(&binding.value)?,
            };
            let value_type = value.ty.clone();
            block_checker.env.define(binding.name.clone(), value_type);
            bindings.push(TypedLetBinding {
                name: binding.name.clone(),
                declared_type: binding.opt_type.clone(),
                value,
            });
        }

        let exprs = block
            .exprs
            .iter()
            .map(|expr| block_checker.infer_expr(expr))
            .collect::<TypeResult<Vec<_>>>()?;
        let trailing_expr = block
            .trailing_expr
            .as_ref()
            .map(|expr| block_checker.infer_expr(expr).map(Box::new))
            .transpose()?;
        let ty = trailing_expr
            .as_ref()
            .map(|expr| expr.ty.clone())
            .or_else(|| exprs.last().map(|expr| expr.ty.clone()))
            .unwrap_or(Type::Unit);

        Ok(TypedExpr::new(
            TypedExprKind::DoBlock {
                bindings,
                exprs,
                trailing_expr,
            },
            ty,
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
