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
export declare function typeToString(t: CalyndaType): string;
export declare function typesCompatible(a: CalyndaType, b: CalyndaType): boolean;
