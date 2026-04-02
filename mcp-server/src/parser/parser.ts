import { Lexer, Token, TokenType } from './lexer';
import * as AST from './ast';

export class ParseError extends Error {
  constructor(message: string, public line: number, public column: number) {
    super(`${line}:${column}: ${message}`);
    this.name = 'ParseError';
  }
}

export interface ParseResult {
  ast: AST.Program | null;
  errors: ParseError[];
}

const PRIMITIVE_TYPES = new Set([
  'int8', 'int16', 'int32', 'int64',
  'uint8', 'uint16', 'uint32', 'uint64',
  'float32', 'float64',
  'bool', 'char', 'string',
]);

const MODIFIERS = new Set(['public', 'private', 'final']);

const ASSIGNMENT_OPS = new Set(['=', '+=', '-=', '*=', '/=', '%=', '&=', '|=', '^=', '<<=', '>>=']);

class Parser {
  private tokens: Token[];
  private pos: number = 0;
  private errors: ParseError[] = [];

  constructor(tokens: Token[]) {
    this.tokens = tokens;
  }

  get parseErrors(): ParseError[] { return this.errors; }

  private peek(offset = 0): Token {
    const idx = this.pos + offset;
    return this.tokens[idx] ?? { type: 'eof', value: '', line: 0, column: 0, offset: 0 };
  }

  private advance(): Token {
    const tok = this.tokens[this.pos];
    if (this.pos < this.tokens.length - 1) this.pos++;
    return tok;
  }

  private check(type: TokenType, value?: string): boolean {
    const tok = this.peek();
    if (tok.type !== type) return false;
    if (value !== undefined && tok.value !== value) return false;
    return true;
  }

  private eat(type: TokenType, value?: string): Token {
    if (this.check(type, value)) return this.advance();
    const tok = this.peek();
    const expected = value ? `'${value}'` : type;
    this.errors.push(new ParseError(`Expected ${expected}, got '${tok.value}'`, tok.line, tok.column));
    return { type, value: value ?? '', line: tok.line, column: tok.column, offset: tok.offset };
  }

  private position(): AST.Position {
    const tok = this.peek();
    return { line: tok.line, column: tok.column, offset: tok.offset };
  }

  private tokenToPosition(tok: Token): AST.Position {
    return { line: tok.line, column: tok.column, offset: tok.offset };
  }

  parseProgram(): AST.Program {
    const start = this.position();
    let packageDecl: AST.PackageDecl | undefined;
    const imports: AST.ImportDecl[] = [];
    const declarations: AST.TopLevelDecl[] = [];

    if (this.check('keyword', 'package')) {
      packageDecl = this.parsePackageDecl();
    }

    while (this.check('keyword', 'import')) {
      imports.push(this.parseImportDecl());
    }

    while (!this.check('eof')) {
      try {
        const decl = this.parseTopLevelDecl();
        if (decl) declarations.push(decl);
      } catch (e) {
        if (e instanceof ParseError) {
          this.errors.push(e);
          while (!this.check('eof') && !this.check('semicolon')) this.advance();
          if (this.check('semicolon')) this.advance();
        } else throw e;
      }
    }

    const end = this.position();
    return { kind: 'Program', packageDecl, imports, declarations, start, end };
  }

  private parsePackageDecl(): AST.PackageDecl {
    const startTok = this.eat('keyword', 'package');
    const name = this.parseQualifiedName();
    this.eat('semicolon');
    return { kind: 'PackageDecl', name, start: this.tokenToPosition(startTok), end: this.position() };
  }

  private parseImportDecl(): AST.ImportDecl {
    const startTok = this.eat('keyword', 'import');
    const name = this.parseQualifiedName();
    this.eat('semicolon');
    return { kind: 'ImportDecl', name, start: this.tokenToPosition(startTok), end: this.position() };
  }

  private parseQualifiedName(): string {
    let name = this.eat('identifier').value;
    while (this.check('dot')) {
      this.advance();
      name += '.' + this.eat('identifier').value;
    }
    return name;
  }

  private parseTopLevelDecl(): AST.TopLevelDecl | null {
    if (this.check('keyword', 'start')) {
      return this.parseStartDecl();
    }

    const modifiers: string[] = [];
    while (this.check('keyword') && MODIFIERS.has(this.peek().value)) {
      modifiers.push(this.advance().value);
    }

    if (this.check('keyword', 'var') || this.check('type') || this.check('keyword', 'void')) {
      return this.parseBindingDecl(modifiers);
    }

    const tok = this.peek();
    throw new ParseError(`Unexpected token '${tok.value}'`, tok.line, tok.column);
  }

  private parseStartDecl(): AST.StartDecl {
    const startTok = this.eat('keyword', 'start');
    this.eat('lparen');
    const params = this.parseParameterList();
    this.eat('rparen');
    this.eat('arrow');
    const body = this.parseLambdaBody();
    this.eat('semicolon');
    return { kind: 'StartDecl', params, body, start: this.tokenToPosition(startTok), end: this.position() };
  }

  private parseBindingDecl(modifiers: string[]): AST.BindingDecl {
    const startPos = this.position();
    let typeAnnotation: AST.TypeNode | 'var';

    if (this.check('keyword', 'var')) {
      this.advance();
      typeAnnotation = 'var';
    } else {
      typeAnnotation = this.parseType();
    }

    const name = this.eat('identifier').value;
    this.eat('eq');
    const value = this.parseExpression();
    this.eat('semicolon');

    return { kind: 'BindingDecl', modifiers, typeAnnotation, name, value, start: startPos, end: this.position() };
  }

  private parseType(): AST.TypeNode {
    const startPos = this.position();

    if (this.check('keyword', 'void')) {
      this.advance();
      const end = this.position();
      const voidNode: AST.VoidTypeNode = { kind: 'VoidType', start: startPos, end };
      return voidNode;
    }

    const typeTok = this.eat('type');
    let typeNode: AST.TypeNode = { kind: 'PrimitiveType', name: typeTok.value, start: this.tokenToPosition(typeTok), end: this.position() };

    while (this.check('lbracket')) {
      const arrStart = this.position();
      this.advance();
      let size: number | undefined;
      if (this.check('integer')) {
        size = parseInt(this.advance().value, 10);
      }
      this.eat('rbracket');
      typeNode = { kind: 'ArrayType', elementType: typeNode, size, start: arrStart, end: this.position() };
    }

    return typeNode;
  }

  private parseParameterList(): AST.Parameter[] {
    const params: AST.Parameter[] = [];
    if (this.check('rparen')) return params;

    params.push(this.parseParameter());
    while (this.check('comma')) {
      this.advance();
      params.push(this.parseParameter());
    }
    return params;
  }

  private parseParameter(): AST.Parameter {
    const startPos = this.position();
    const typeAnnotation = this.parseType();
    const name = this.eat('identifier').value;
    return { kind: 'Parameter', typeAnnotation, name, start: startPos, end: this.position() };
  }

  private parseLambdaBody(): AST.Block | AST.Expression {
    if (this.check('lbrace')) return this.parseBlock();
    return this.parseExpression();
  }

  private parseBlock(): AST.Block {
    const startPos = this.position();
    this.eat('lbrace');
    const statements: AST.Statement[] = [];
    while (!this.check('rbrace') && !this.check('eof')) {
      try {
        const stmt = this.parseStatement();
        if (stmt) statements.push(stmt);
      } catch (e) {
        if (e instanceof ParseError) {
          this.errors.push(e);
          while (!this.check('eof') && !this.check('semicolon') && !this.check('rbrace')) this.advance();
          if (this.check('semicolon')) this.advance();
        } else throw e;
      }
    }
    this.eat('rbrace');
    return { kind: 'Block', statements, start: startPos, end: this.position() };
  }

  private parseStatement(): AST.Statement {
    const startPos = this.position();
    const tok = this.peek();

    if (tok.type === 'keyword') {
      switch (tok.value) {
        case 'return': {
          this.advance();
          let value: AST.Expression | undefined;
          if (!this.check('semicolon')) value = this.parseExpression();
          this.eat('semicolon');
          return { kind: 'ReturnStatement', value, start: startPos, end: this.position() };
        }
        case 'exit': {
          this.advance();
          this.eat('semicolon');
          return { kind: 'ExitStatement', start: startPos, end: this.position() };
        }
        case 'throw': {
          this.advance();
          const value = this.parseExpression();
          this.eat('semicolon');
          return { kind: 'ThrowStatement', value, start: startPos, end: this.position() };
        }
        case 'var': {
          this.advance();
          const name = this.eat('identifier').value;
          this.eat('eq');
          const value = this.parseExpression();
          this.eat('semicolon');
          return { kind: 'LocalBindingStatement', final: false, typeAnnotation: 'var', name, value, start: startPos, end: this.position() };
        }
        case 'final': {
          this.advance();
          const typeAnnotation = this.parseType();
          const name = this.eat('identifier').value;
          this.eat('eq');
          const value = this.parseExpression();
          this.eat('semicolon');
          return { kind: 'LocalBindingStatement', final: true, typeAnnotation, name, value, start: startPos, end: this.position() };
        }
      }
    }

    // Check for local binding: type identifier = expr;
    if ((tok.type === 'type' || (tok.type === 'keyword' && tok.value === 'void'))
      && this.peek(1).type === 'identifier'
      && this.peek(2).type !== 'lparen') {
      const typeAnnotation = this.parseType();
      if (this.check('identifier') && (this.peek(1).type === 'eq' || this.peek(1).type === 'semicolon')) {
        const name = this.advance().value;
        this.eat('eq');
        const value = this.parseExpression();
        this.eat('semicolon');
        return { kind: 'LocalBindingStatement', final: false, typeAnnotation, name, value, start: startPos, end: this.position() };
      }
      throw new ParseError(`Unexpected type annotation in statement position`, tok.line, tok.column);
    }

    // Expression statement
    const expr = this.parseExpression();
    this.eat('semicolon');
    return { kind: 'ExpressionStatement', expression: expr, start: startPos, end: this.position() };
  }

  private parseExpression(): AST.Expression {
    if (this.isLambdaAhead()) return this.parseLambdaExpression();
    return this.parseAssignmentExpression();
  }

  private isLambdaAhead(): boolean {
    if (!this.check('lparen')) return false;
    let i = 1;
    let depth = 1;
    while (this.pos + i < this.tokens.length && depth > 0) {
      const t = this.tokens[this.pos + i];
      if (t.type === 'lparen') depth++;
      else if (t.type === 'rparen') depth--;
      i++;
    }
    return this.tokens[this.pos + i]?.type === 'arrow';
  }

  private parseLambdaExpression(): AST.LambdaExpression {
    const startPos = this.position();
    this.eat('lparen');
    const params = this.parseParameterList();
    this.eat('rparen');
    this.eat('arrow');
    const body = this.parseLambdaBody();
    return { kind: 'LambdaExpression', params, body, start: startPos, end: this.position() };
  }

  private parseAssignmentExpression(): AST.Expression {
    const left = this.parseTernaryExpression();
    const tok = this.peek();
    if (ASSIGNMENT_OPS.has(tok.value) && (tok.type === 'eq' || tok.type.endsWith('eq'))) {
      const op = this.advance().value;
      const right = this.parseAssignmentExpression();
      return { kind: 'AssignmentExpression', operator: op, left, right, start: left.start, end: right.end };
    }
    return left;
  }

  private parseTernaryExpression(): AST.Expression {
    const cond = this.parseLogicalOr();
    if (this.check('question')) {
      this.advance();
      const consequent = this.parseExpression();
      this.eat('colon');
      const alternate = this.parseTernaryExpression();
      return { kind: 'TernaryExpression', condition: cond, consequent, alternate, start: cond.start, end: alternate.end };
    }
    return cond;
  }

  private parseLogicalOr(): AST.Expression {
    let left = this.parseLogicalAnd();
    while (this.check('pipepipe')) {
      const op = this.advance().value;
      const right = this.parseLogicalAnd();
      left = { kind: 'BinaryExpression', operator: op, left, right, start: left.start, end: right.end };
    }
    return left;
  }

  private parseLogicalAnd(): AST.Expression {
    let left = this.parseBitwiseOr();
    while (this.check('ampamp')) {
      const op = this.advance().value;
      const right = this.parseBitwiseOr();
      left = { kind: 'BinaryExpression', operator: op, left, right, start: left.start, end: right.end };
    }
    return left;
  }

  private parseBitwiseOr(): AST.Expression {
    let left = this.parseBitwiseNand();
    while (this.check('pipe')) {
      const op = this.advance().value;
      const right = this.parseBitwiseNand();
      left = { kind: 'BinaryExpression', operator: op, left, right, start: left.start, end: right.end };
    }
    return left;
  }

  private parseBitwiseNand(): AST.Expression {
    let left = this.parseBitwiseXor();
    while (this.check('tildeamp')) {
      const op = this.advance().value;
      const right = this.parseBitwiseXor();
      left = { kind: 'BinaryExpression', operator: op, left, right, start: left.start, end: right.end };
    }
    return left;
  }

  private parseBitwiseXor(): AST.Expression {
    let left = this.parseBitwiseXnor();
    while (this.check('caret')) {
      const op = this.advance().value;
      const right = this.parseBitwiseXnor();
      left = { kind: 'BinaryExpression', operator: op, left, right, start: left.start, end: right.end };
    }
    return left;
  }

  private parseBitwiseXnor(): AST.Expression {
    let left = this.parseBitwiseAnd();
    while (this.check('tildecaret')) {
      const op = this.advance().value;
      const right = this.parseBitwiseAnd();
      left = { kind: 'BinaryExpression', operator: op, left, right, start: left.start, end: right.end };
    }
    return left;
  }

  private parseBitwiseAnd(): AST.Expression {
    let left = this.parseEquality();
    while (this.check('amp')) {
      const op = this.advance().value;
      const right = this.parseEquality();
      left = { kind: 'BinaryExpression', operator: op, left, right, start: left.start, end: right.end };
    }
    return left;
  }

  private parseEquality(): AST.Expression {
    let left = this.parseRelational();
    while (this.check('eqeq') || this.check('bangeq')) {
      const op = this.advance().value;
      const right = this.parseRelational();
      left = { kind: 'BinaryExpression', operator: op, left, right, start: left.start, end: right.end };
    }
    return left;
  }

  private parseRelational(): AST.Expression {
    let left = this.parseShift();
    while (this.check('lt') || this.check('gt') || this.check('lteq') || this.check('gteq')) {
      const op = this.advance().value;
      const right = this.parseShift();
      left = { kind: 'BinaryExpression', operator: op, left, right, start: left.start, end: right.end };
    }
    return left;
  }

  private parseShift(): AST.Expression {
    let left = this.parseAdditive();
    while (this.check('ltlt') || this.check('gtgt')) {
      const op = this.advance().value;
      const right = this.parseAdditive();
      left = { kind: 'BinaryExpression', operator: op, left, right, start: left.start, end: right.end };
    }
    return left;
  }

  private parseAdditive(): AST.Expression {
    let left = this.parseMultiplicative();
    while (this.check('plus') || this.check('minus')) {
      const op = this.advance().value;
      const right = this.parseMultiplicative();
      left = { kind: 'BinaryExpression', operator: op, left, right, start: left.start, end: right.end };
    }
    return left;
  }

  private parseMultiplicative(): AST.Expression {
    let left = this.parseUnary();
    while (this.check('star') || this.check('slash') || this.check('percent')) {
      const op = this.advance().value;
      const right = this.parseUnary();
      left = { kind: 'BinaryExpression', operator: op, left, right, start: left.start, end: right.end };
    }
    return left;
  }

  private parseUnary(): AST.Expression {
    const startPos = this.position();
    if (this.check('bang') || this.check('tilde') || this.check('minus') || this.check('plus')) {
      const op = this.advance().value;
      const operand = this.parseUnary();
      return { kind: 'UnaryExpression', operator: op, operand, start: startPos, end: this.position() };
    }
    return this.parsePostfix();
  }

  private parsePostfix(): AST.Expression {
    let expr = this.parsePrimary();
    while (true) {
      if (this.check('lparen')) {
        this.advance();
        const args: AST.Expression[] = [];
        if (!this.check('rparen')) {
          args.push(this.parseExpression());
          while (this.check('comma')) {
            this.advance();
            args.push(this.parseExpression());
          }
        }
        this.eat('rparen');
        expr = { kind: 'CallExpression', callee: expr, args, start: expr.start, end: this.position() };
      } else if (this.check('lbracket')) {
        this.advance();
        const index = this.parseExpression();
        this.eat('rbracket');
        expr = { kind: 'IndexExpression', object: expr, index, start: expr.start, end: this.position() };
      } else if (this.check('dot')) {
        this.advance();
        const member = this.eat('identifier').value;
        expr = { kind: 'MemberExpression', object: expr, member, start: expr.start, end: this.position() };
      } else {
        break;
      }
    }
    return expr;
  }

  private parsePrimary(): AST.Expression {
    const startPos = this.position();
    const tok = this.peek();

    if (tok.type === 'null') {
      this.advance();
      return { kind: 'NullLiteral', start: startPos, end: this.position() };
    }

    if (tok.type === 'bool') {
      this.advance();
      return { kind: 'BoolLiteral', value: tok.value === 'true', start: startPos, end: this.position() };
    }

    if (tok.type === 'integer') {
      this.advance();
      const raw = tok.value;
      let val: number;
      if (raw.startsWith('0b') || raw.startsWith('0B')) val = parseInt(raw.slice(2), 2);
      else if (raw.startsWith('0x') || raw.startsWith('0X')) val = parseInt(raw.slice(2), 16);
      else if (raw.startsWith('0o') || raw.startsWith('0O')) val = parseInt(raw.slice(2), 8);
      else val = parseInt(raw, 10);
      return { kind: 'IntegerLiteral', value: val, raw, start: startPos, end: this.position() };
    }

    if (tok.type === 'float') {
      this.advance();
      return { kind: 'FloatLiteral', value: parseFloat(tok.value), raw: tok.value, start: startPos, end: this.position() };
    }

    if (tok.type === 'string') {
      this.advance();
      const raw = tok.value.slice(1, -1);
      return { kind: 'StringLiteral', value: raw, start: startPos, end: this.position() };
    }

    if (tok.type === 'char') {
      this.advance();
      let val = tok.value.slice(1, -1);
      if (val.startsWith('\\')) {
        switch (val[1]) {
          case 'n': val = '\n'; break;
          case 't': val = '\t'; break;
          case 'r': val = '\r'; break;
          case '\\': val = '\\'; break;
          case "'": val = "'"; break;
          default: break;
        }
      }
      return { kind: 'CharLiteral', value: val, start: startPos, end: this.position() };
    }

    if (tok.type === 'template_literal') {
      this.advance();
      const parts = this.parseTemplateParts(tok.value);
      return { kind: 'TemplateLiteral', parts, start: startPos, end: this.position() };
    }

    if (tok.type === 'lbracket') {
      this.advance();
      const elements: AST.Expression[] = [];
      if (!this.check('rbracket')) {
        elements.push(this.parseExpression());
        while (this.check('comma')) {
          this.advance();
          elements.push(this.parseExpression());
        }
      }
      this.eat('rbracket');
      return { kind: 'ArrayLiteral', elements, start: startPos, end: this.position() };
    }

    if (tok.type === 'lparen') {
      this.advance();
      const expr = this.parseExpression();
      this.eat('rparen');
      return expr;
    }

    // Cast expression: PrimitiveType ( expr )
    if (tok.type === 'type' && this.peek(1).type === 'lparen') {
      const typeName = this.advance().value;
      this.advance(); // (
      const value = this.parseExpression();
      this.eat('rparen');
      return { kind: 'CastExpression', targetType: typeName, value, start: startPos, end: this.position() };
    }

    if (tok.type === 'identifier') {
      this.advance();
      return { kind: 'Identifier', name: tok.value, start: startPos, end: this.position() };
    }

    // throw in expression context (e.g. ternary branch): model as unary
    if (tok.type === 'keyword' && tok.value === 'throw') {
      this.advance();
      const value = this.parseExpression();
      return { kind: 'UnaryExpression', operator: 'throw', operand: value, start: startPos, end: this.position() };
    }

    throw new ParseError(`Unexpected token '${tok.value}' (${tok.type})`, tok.line, tok.column);
  }

  private parseTemplateParts(raw: string): (string | AST.Expression)[] {
    const content = raw.slice(1, -1); // remove surrounding backticks
    const parts: (string | AST.Expression)[] = [];
    let i = 0;
    let str = '';
    while (i < content.length) {
      if (content[i] === '$' && content[i + 1] === '{') {
        if (str) { parts.push(str); str = ''; }
        i += 2;
        let depth = 1;
        let exprSrc = '';
        while (i < content.length && depth > 0) {
          if (content[i] === '{') { depth++; exprSrc += content[i++]; }
          else if (content[i] === '}') { depth--; if (depth > 0) exprSrc += content[i]; i++; }
          else { exprSrc += content[i++]; }
        }
        try {
          const innerLexer = new Lexer(exprSrc);
          const innerTokens = innerLexer.tokenize();
          const innerParser = new Parser(innerTokens);
          const expr = innerParser.parseExpression();
          parts.push(expr);
        } catch {
          parts.push('${' + exprSrc + '}');
        }
      } else if (content[i] === '\\' && i + 1 < content.length) {
        str += content[i] + content[i + 1];
        i += 2;
      } else {
        str += content[i++];
      }
    }
    if (str) parts.push(str);
    return parts;
  }
}

export function parse(tokens: Token[]): ParseResult {
  const parser = new Parser(tokens);
  try {
    const ast = parser.parseProgram();
    return { ast, errors: parser.parseErrors };
  } catch (e) {
    if (e instanceof ParseError) {
      return { ast: null, errors: [e, ...parser.parseErrors] };
    }
    throw e;
  }
}
