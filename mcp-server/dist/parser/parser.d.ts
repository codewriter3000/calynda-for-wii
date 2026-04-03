import { Token } from './lexer';
import * as AST from './ast';
export declare class ParseError extends Error {
    line: number;
    column: number;
    constructor(message: string, line: number, column: number);
}
export interface ParseResult {
    ast: AST.Program | null;
    errors: ParseError[];
}
export declare function parse(tokens: Token[]): ParseResult;
