use crate::ast::Expr;
use crate::ast::types::Type;

/// Represents an Objective-C style selector.
#[derive(Debug, Clone, PartialEq)]
pub struct Selector(pub String);

/// An Objective-C style message of the format:
/// `[target selector-name arg1:val arg2:val2 ...]`
#[derive(Debug, Clone, PartialEq)]
pub struct Message {
    pub target: String,
    pub selector: Selector,
    pub keyword_args: Vec<(String, Type)>,
    pub return_type: Option<Type>,
}

/// A handler for an Objective-C style message, containing the message itself
/// and the body of the handler.
#[derive(Debug, Clone, PartialEq)]
pub struct MessageHandler {
    pub message: Message,
    pub handler_body: Expr,
}
