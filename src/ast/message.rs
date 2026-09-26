use crate::ast::Expr;
use crate::ast::types::Type;

/// Represents an Objective-C style selector.
#[derive(Debug, Clone, PartialEq)]
pub struct Selector(pub String);

/// Represents an Objective-C style message of the format:
/// `@(target selector-name arg1:val)`
#[derive(Debug, Clone, PartialEq)]
pub struct Message {
    pub selector: Selector,
    pub target: String,
    pub args: Vec<(String, Type)>,
    pub return_type: Option<Type>,
}

/// Represents a handler for an Objective-C style message, containing the message itself
/// and the body of the handler.
#[derive(Debug, Clone, PartialEq)]
pub struct MessageHandler {
    pub message: Message,
    pub handler_body: Expr,
}
