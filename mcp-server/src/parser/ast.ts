export type NodeKind = 
  | 'Program' | 'PackageDecl' | 'ImportDecl'
  | 'StartDecl' | 'BindingDecl' | 'UnionDecl'
  | 'Block' | 'LocalBindingStatement' | 'ReturnStatement' | 'ExitStatement' | 'ThrowStatement' | 'ExpressionStatement'
  | 'LambdaExpression' | 'AssignmentExpression' | 'TernaryExpression'
  | 'BinaryExpression' | 'UnaryExpression' | 'PostfixExpression' | 'PostfixIncrementExpression' | 'PostfixDecrementExpression'
  | 'CallExpression' | 'IndexExpression' | 'MemberExpression'
  | 'CastExpression' | 'ArrayLiteral' | 'DiscardExpression'
  | 'Identifier' | 'IntegerLiteral' | 'FloatLiteral' | 'BoolLiteral' | 'CharLiteral' | 'StringLiteral' | 'TemplateLiteral' | 'NullLiteral'
  | 'Parameter' | 'ParameterList' | 'ArrayType' | 'PrimitiveType' | 'NamedType' | 'VoidType'
  | 'HeterogeneousArrayType' | 'WildcardType'
  ;

export interface Position {
  line: number;
  column: number;
  offset: number;
}

export interface ASTNode {
  kind: NodeKind;
  start: Position;
  end: Position;
}

export interface Program extends ASTNode {
  kind: 'Program';
  packageDecl?: PackageDecl;
  imports: ImportDecl[];
  declarations: TopLevelDecl[];
}

export interface PackageDecl extends ASTNode {
  kind: 'PackageDecl';
  name: string;
}

export interface ImportDecl extends ASTNode {
  kind: 'ImportDecl';
  /** Fully-qualified module name (e.g. "foo.bar") */
  name: string;
  /** V2 import form */
  form: 'plain' | 'alias' | 'wildcard' | 'selective';
  /** Alias name for `import foo.bar as baz` */
  alias?: string;
  /** Selected names for `import foo.bar.{a, b, c}` */
  selected?: string[];
}

export type TopLevelDecl = StartDecl | BindingDecl | UnionDecl;

export interface UnionVariant {
  name: string;
  payloadType?: TypeNode;
}

export interface UnionDecl extends ASTNode {
  kind: 'UnionDecl';
  modifiers: string[];
  name: string;
  genericParams: string[];
  variants: UnionVariant[];
}

export interface StartDecl extends ASTNode {
  kind: 'StartDecl';
  params: Parameter[];
  body: Block | Expression;
}

export interface BindingDecl extends ASTNode {
  kind: 'BindingDecl';
  modifiers: string[];
  typeAnnotation: TypeNode | 'var';
  name: string;
  value: Expression;
}

export type TypeNode = PrimitiveTypeNode | ArrayTypeNode | NamedTypeNode | VoidTypeNode | HeterogeneousArrayTypeNode | WildcardTypeNode;

export interface PrimitiveTypeNode extends ASTNode {
  kind: 'PrimitiveType';
  name: string;
}

export interface ArrayTypeNode extends ASTNode {
  kind: 'ArrayType';
  elementType: TypeNode;
  size?: number;
}

export interface NamedTypeNode extends ASTNode {
  kind: 'NamedType';
  name: string;
  genericArgs: TypeNode[];
}

export interface VoidTypeNode extends ASTNode {
  kind: 'VoidType';
}

/** V2: heterogeneous array type `arr<?>` or `arr<T>` */
export interface HeterogeneousArrayTypeNode extends ASTNode {
  kind: 'HeterogeneousArrayType';
  genericArgs: TypeNode[];
}

/** V2: wildcard generic type argument `?` */
export interface WildcardTypeNode extends ASTNode {
  kind: 'WildcardType';
}

export interface Parameter extends ASTNode {
  kind: 'Parameter';
  typeAnnotation: TypeNode;
  name: string;
  /** V2: true when the parameter is variadic (`Type... name`) */
  isVarargs: boolean;
}

export interface Block extends ASTNode {
  kind: 'Block';
  statements: Statement[];
}

export type Statement = LocalBindingStatement | ReturnStatement | ExitStatement | ThrowStatement | ExpressionStatement;

export interface LocalBindingStatement extends ASTNode {
  kind: 'LocalBindingStatement';
  final: boolean;
  typeAnnotation: TypeNode | 'var';
  name: string;
  value: Expression;
}

export interface ReturnStatement extends ASTNode {
  kind: 'ReturnStatement';
  value?: Expression;
}

export interface ExitStatement extends ASTNode {
  kind: 'ExitStatement';
}

export interface ThrowStatement extends ASTNode {
  kind: 'ThrowStatement';
  value: Expression;
}

export interface ExpressionStatement extends ASTNode {
  kind: 'ExpressionStatement';
  expression: Expression;
}

export type Expression =
  | LambdaExpression | AssignmentExpression | TernaryExpression
  | BinaryExpression | UnaryExpression
  | CallExpression | IndexExpression | MemberExpression
  | CastExpression | ArrayLiteralNode | DiscardExpressionNode
  | PostfixIncrementExpressionNode | PostfixDecrementExpressionNode
  | Identifier | IntegerLiteralNode | FloatLiteralNode | BoolLiteralNode | CharLiteralNode | StringLiteralNode | TemplateLiteralNode | NullLiteralNode;

export interface LambdaExpression extends ASTNode {
  kind: 'LambdaExpression';
  params: Parameter[];
  body: Block | Expression;
}

export interface AssignmentExpression extends ASTNode {
  kind: 'AssignmentExpression';
  operator: string;
  left: Expression;
  right: Expression;
}

export interface TernaryExpression extends ASTNode {
  kind: 'TernaryExpression';
  condition: Expression;
  consequent: Expression;
  alternate: Expression;
}

export interface BinaryExpression extends ASTNode {
  kind: 'BinaryExpression';
  operator: string;
  left: Expression;
  right: Expression;
}

export interface UnaryExpression extends ASTNode {
  kind: 'UnaryExpression';
  operator: string;
  operand: Expression;
}

export interface CallExpression extends ASTNode {
  kind: 'CallExpression';
  callee: Expression;
  args: Expression[];
}

export interface IndexExpression extends ASTNode {
  kind: 'IndexExpression';
  object: Expression;
  index: Expression;
}

export interface MemberExpression extends ASTNode {
  kind: 'MemberExpression';
  object: Expression;
  member: string;
}

export interface CastExpression extends ASTNode {
  kind: 'CastExpression';
  targetType: string;
  value: Expression;
}

export interface ArrayLiteralNode extends ASTNode {
  kind: 'ArrayLiteral';
  elements: Expression[];
}

/** V2: discard expression `_` — signals that a value is intentionally unused */
export interface DiscardExpressionNode extends ASTNode {
  kind: 'DiscardExpression';
}

/** V2: postfix increment `expr++` */
export interface PostfixIncrementExpressionNode extends ASTNode {
  kind: 'PostfixIncrementExpression';
  operand: Expression;
}

/** V2: postfix decrement `expr--` */
export interface PostfixDecrementExpressionNode extends ASTNode {
  kind: 'PostfixDecrementExpression';
  operand: Expression;
}

export interface Identifier extends ASTNode {
  kind: 'Identifier';
  name: string;
}

export interface IntegerLiteralNode extends ASTNode {
  kind: 'IntegerLiteral';
  value: number;
  raw: string;
}

export interface FloatLiteralNode extends ASTNode {
  kind: 'FloatLiteral';
  value: number;
  raw: string;
}

export interface BoolLiteralNode extends ASTNode {
  kind: 'BoolLiteral';
  value: boolean;
}

export interface CharLiteralNode extends ASTNode {
  kind: 'CharLiteral';
  value: string;
}

export interface StringLiteralNode extends ASTNode {
  kind: 'StringLiteral';
  value: string;
}

export interface TemplateLiteralNode extends ASTNode {
  kind: 'TemplateLiteral';
  parts: (string | Expression)[];
}

export interface NullLiteralNode extends ASTNode {
  kind: 'NullLiteral';
}
