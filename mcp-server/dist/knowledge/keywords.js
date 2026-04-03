"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.ALL_RESERVED = exports.PRIMITIVE_TYPES = exports.KEYWORDS = void 0;
exports.KEYWORDS = [
    'package', 'import', 'public', 'private', 'final', 'var', 'start',
    'return', 'exit', 'throw', 'null', 'true', 'false', 'void',
];
exports.PRIMITIVE_TYPES = [
    'int8', 'int16', 'int32', 'int64',
    'uint8', 'uint16', 'uint32', 'uint64',
    'float32', 'float64',
    'bool', 'char', 'string',
];
exports.ALL_RESERVED = [...exports.KEYWORDS, ...exports.PRIMITIVE_TYPES];
//# sourceMappingURL=keywords.js.map