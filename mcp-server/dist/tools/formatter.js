"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.formatCode = formatCode;
function formatCode(input) {
    let code = input.code;
    const original = code;
    code = code.replace(/\r\n/g, '\n').replace(/\r/g, '\n');
    code = code.split('\n').map(line => line.trimEnd()).join('\n');
    if (!code.endsWith('\n'))
        code += '\n';
    return { formatted: code, changed: code !== original };
}
//# sourceMappingURL=formatter.js.map