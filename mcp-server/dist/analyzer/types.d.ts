import { PRIMITIVE_TYPES } from '../knowledge/keywords';
export type CalyndaType = PrimitiveCalyndaType | ArrayCalyndaType | NamedCalyndaType | VoidCalyndaType | UnknownType | LambdaCalyndaType | HeterogeneousArrayCalyndaType | WildcardCalyndaType | UnionCalyndaType;
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
export declare function typeToString(t: CalyndaType): string;
export declare function typesCompatible(a: CalyndaType, b: CalyndaType): boolean;
