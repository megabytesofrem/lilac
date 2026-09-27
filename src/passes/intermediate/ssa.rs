//! Static Single Assignment (SSA) pass for the Lilac compiler.

use crate::ast::{
    Literal,
    operator::{BinaryOp, UnaryOp},
    types::Type,
};

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub struct ValueId(pub u32);

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub struct BlockId(pub u32);

#[derive(Debug, Clone, PartialEq)]
pub struct Function {
    pub name: String,
    pub params: Vec<(ValueId, Type)>,
    pub return_type: Type,
    pub blocks: Vec<BasicBlock>,
}

#[derive(Debug, Clone, PartialEq)]
pub struct BasicBlock {
    pub id: BlockId,
    pub params: Vec<(ValueId, Type)>,
    pub instructions: Vec<Instruction>,
    pub terminator: Terminator,
}

#[derive(Debug, Clone, PartialEq)]
pub enum Instruction {
    // %result = <literal value>
    Const {
        result: ValueId,
        value: Literal,
    },

    // %result = <unary operation> %operand
    Unary {
        result: ValueId,
        op: UnaryOp,
        operand: ValueId,
    },

    // %result = %lhs <binary operation> %rhs
    Binary {
        result: ValueId,
        op: BinaryOp,
        lhs: ValueId,
        rhs: ValueId,
    },

    // %result = call callee(%arguments)
    Call {
        result: ValueId,
        callee: ValueId,
        arguments: Vec<ValueId>,
    },

    // %result = call callee(%arguments) with a direct name
    DirectCall {
        result: ValueId,
        callee: String,
        arguments: Vec<ValueId>,
    },

    // Objective-C style message send
    //
    // %result = send_message %curr_win
    //      selector "resize"
    //      args to-width: %width and-height: %height
    SendMessage {
        result: ValueId,
        receiver: ValueId, // The object receiving the message
        protocol: Option<String>,
        selector: String, // The selector name e.g "resize"
        arguments: Vec<(String, ValueId)>,
    },

    // %result = make_array(%elements)
    MakeArray {
        result: ValueId,
        elements: Vec<ValueId>,
    },

    // %result = make_tuple(%elements)
    MakeTuple {
        result: ValueId,
        elements: Vec<ValueId>,
    },

    // %result = %object.%field
    LoadField {
        result: ValueId,
        object: ValueId,
        field: String,
    },

    // %object.%field = %value
    UpdateField {
        object: ValueId,
        field: String,
        value: ValueId,
    },

    // MARK: Retain/release
    // lilac_retain %value
    Retain {
        value: ValueId,
    },

    // lilac_release %value
    Release {
        value: ValueId,
    },
}

#[derive(Debug, Clone, PartialEq)]
pub enum Terminator {
    // return %value
    Return(Option<ValueId>),

    // jump target(%arguments)
    Jump {
        target: BlockId,
        arguments: Vec<ValueId>,
    },

    // branch(condition, then_block(%then_arguments), else_block(%else_arguments))
    Branch {
        condition: ValueId,
        then_block: BlockId,
        then_arguments: Vec<ValueId>,
        else_block: BlockId,
        else_arguments: Vec<ValueId>,
    },

    Unreachable,
}
