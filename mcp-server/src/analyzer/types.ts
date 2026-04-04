import { PRIMITIVE_TYPES } from '../knowledge/keywords';

export type CalyndaType =
  | PrimitiveCalyndaType
  | ArrayCalyndaType
  | NamedCalyndaType
  | VoidCalyndaType
  | UnknownType
  | LambdaCalyndaType
  | HeterogeneousArrayCalyndaType
  | WildcardCalyndaType
  | UnionCalyndaType;

export interface PrimitiveCalyndaType {
  kind: 'primitive';
  name: typeof PRIMITIVE_TYPES[number];
}

export interface ArrayCalyndaType {
  kind: 'array';
  elementType: CalyndaType;
}

export interface NamedCalyndaType {
  kind: 'named';
  name: string;
  genericArgs: CalyndaType[];
}

export interface VoidCalyndaType {
  kind: 'void';
}

export interface LambdaCalyndaType {
  kind: 'lambda';
  params: CalyndaType[];
  returnType: CalyndaType;
}

export interface UnknownType {
  kind: 'unknown';
}

/** V2: heterogeneous array type `arr<?>` or `arr<T>` */
export interface HeterogeneousArrayCalyndaType {
  kind: 'heterogeneous_array';
  genericArgs: CalyndaType[];
}

/** V2: wildcard generic type argument `?` */
export interface WildcardCalyndaType {
  kind: 'wildcard';
}

/** V2: tagged union type */
export interface UnionCalyndaType {
  kind: 'union';
  name: string;
  genericParams: string[];
}

export function typeToString(t: CalyndaType): string {
  switch (t.kind) {
    case 'primitive': return t.name;
    case 'array': return `${typeToString(t.elementType)}[]`;
    case 'named': return t.genericArgs.length > 0
      ? `${t.name}<${t.genericArgs.map(typeToString).join(', ')}>`
      : t.name;
    case 'void': return 'void';
    case 'lambda': return `(${t.params.map(typeToString).join(', ')}) -> ${typeToString(t.returnType)}`;
    case 'unknown': return 'unknown';
    case 'heterogeneous_array': return t.genericArgs.length > 0
      ? `arr<${t.genericArgs.map(typeToString).join(', ')}>`
      : 'arr<?>';
    case 'wildcard': return '?';
    case 'union': return t.genericParams.length > 0
      ? `${t.name}<${t.genericParams.join(', ')}>`
      : t.name;
  }
}

export function typesCompatible(a: CalyndaType, b: CalyndaType): boolean {
  if (a.kind === 'unknown' || b.kind === 'unknown') return true;
  if (a.kind === 'wildcard' || b.kind === 'wildcard') return true;
  if (a.kind !== b.kind) return false;
  if (a.kind === 'primitive' && b.kind === 'primitive') return a.name === b.name;
  if (a.kind === 'array' && b.kind === 'array') return typesCompatible(a.elementType, b.elementType);
  if (a.kind === 'named' && b.kind === 'named') {
    if (a.name !== b.name || a.genericArgs.length !== b.genericArgs.length) return false;
    return a.genericArgs.every((arg, i) => typesCompatible(arg, b.genericArgs[i]));
  }
  if (a.kind === 'heterogeneous_array' && b.kind === 'heterogeneous_array') return true;
  if (a.kind === 'void' && b.kind === 'void') return true;
  if (a.kind === 'union' && b.kind === 'union') return a.name === b.name;
  return false;
}
