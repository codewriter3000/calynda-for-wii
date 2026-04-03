"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.validateCode = validateCode;
const lexer_1 = require("../parser/lexer");
const parser_1 = require("../parser/parser");
const semantic_1 = require("../analyzer/semantic");
const diagnostics_1 = require("../analyzer/diagnostics");
function validateCode(input) {
    const lexer = new lexer_1.Lexer(input.code);
    const tokens = lexer.tokenize();
    const parseResult = (0, parser_1.parse)(tokens);
    const errors = parseResult.errors.map(e => `[ERROR] ${e.line}:${e.column}: ${e.message}`);
    const warnings = [];
    const info = [];
    if (parseResult.ast) {
        const analysis = (0, semantic_1.analyze)(parseResult.ast);
        for (const diag of analysis.diagnostics) {
            const formatted = (0, diagnostics_1.formatDiagnostic)(diag);
            if (diag.severity === 'error')
                errors.push(formatted);
            else if (diag.severity === 'warning')
                warnings.push(formatted);
            else
                info.push(formatted);
        }
    }
    return { valid: errors.length === 0, errors, warnings, info };
}
//# sourceMappingURL=validator.js.map