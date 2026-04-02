import { PRIMITIVE_TYPES } from '../knowledge/keywords';

export type CalyndaType = PrimitiveCalyndaType | ArrayCalyndaType | VoidCalyndaType | UnknownType | LambdaCalyndaType;

export interface PrimitiveCalyndaType {
  kind: 'primitive';
  name: typeof PRIMITIVE_TYPES[number];
}

export interface ArrayCalyndaType {
  kind: 'array';
  elementType: CalyndaType;
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

export function typeToString(t: CalyndaType): string {
  switch (t.kind) {
    case 'primitive': return t.name;
    case 'array': return `${typeToString(t.elementType)}[]`;
    case 'void': return 'void';
    case 'lambda': return `(${t.params.map(typeToString).join(', ')}) -> ${typeToString(t.returnType)}`;
    case 'unknown': return 'unknown';
  }
}

export function typesCompatible(a: CalyndaType, b: CalyndaType): boolean {
  if (a.kind === 'unknown' || b.kind === 'unknown') return true;
  if (a.kind !== b.kind) return false;
  if (a.kind === 'primitive' && b.kind === 'primitive') return a.name === b.name;
  if (a.kind === 'array' && b.kind === 'array') return typesCompatible(a.elementType, b.elementType);
  if (a.kind === 'void' && b.kind === 'void') return true;
  return false;
}
