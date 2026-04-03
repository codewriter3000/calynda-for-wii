export declare const KEYWORDS: readonly ["package", "import", "public", "private", "final", "var", "start", "return", "exit", "throw", "null", "true", "false", "void"];
export declare const PRIMITIVE_TYPES: readonly ["int8", "int16", "int32", "int64", "uint8", "uint16", "uint32", "uint64", "float32", "float64", "bool", "char", "string"];
export declare const ALL_RESERVED: readonly ["package", "import", "public", "private", "final", "var", "start", "return", "exit", "throw", "null", "true", "false", "void", "int8", "int16", "int32", "int64", "uint8", "uint16", "uint32", "uint64", "float32", "float64", "bool", "char", "string"];
export type Keyword = typeof KEYWORDS[number];
export type PrimitiveType = typeof PRIMITIVE_TYPES[number];
