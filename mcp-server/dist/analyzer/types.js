"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.typeToString = typeToString;
exports.typesCompatible = typesCompatible;
function typeToString(t) {
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
function typesCompatible(a, b) {
    if (a.kind === 'unknown' || b.kind === 'unknown')
        return true;
    if (a.kind === 'wildcard' || b.kind === 'wildcard')
        return true;
    if (a.kind !== b.kind)
        return false;
    if (a.kind === 'primitive' && b.kind === 'primitive')
        return a.name === b.name;
    if (a.kind === 'array' && b.kind === 'array')
        return typesCompatible(a.elementType, b.elementType);
    if (a.kind === 'named' && b.kind === 'named') {
        if (a.name !== b.name || a.genericArgs.length !== b.genericArgs.length)
            return false;
        return a.genericArgs.every((arg, i) => typesCompatible(arg, b.genericArgs[i]));
    }
    if (a.kind === 'heterogeneous_array' && b.kind === 'heterogeneous_array')
        return true;
    if (a.kind === 'void' && b.kind === 'void')
        return true;
    if (a.kind === 'union' && b.kind === 'union')
        return a.name === b.name;
    return false;
}
//# sourceMappingURL=types.js.map