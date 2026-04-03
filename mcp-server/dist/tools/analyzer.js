"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.analyzeCode = analyzeCode;
const lexer_1 = require("../parser/lexer");
const parser_1 = require("../parser/parser");
const semantic_1 = require("../analyzer/semantic");
const types_1 = require("../analyzer/types");
function analyzeCode(input) {
    const lexer = new lexer_1.Lexer(input.code);
    const tokens = lexer.tokenize();
    const parseResult = (0, parser_1.parse)(tokens);
    const parseErrors = parseResult.errors.map(e => `${e.line}:${e.column}: ${e.message}`);
    if (!parseResult.ast) {
        return { parseErrors, diagnostics: [] };
    }
    const analysis = (0, semantic_1.analyze)(parseResult.ast);
    const symbols = {};
    for (const [name, type] of analysis.symbols) {
        symbols[name] = (0, types_1.typeToString)(type);
    }
    return {
        ast: parseResult.ast,
        symbols,
        diagnostics: analysis.diagnostics.map(d => `[${d.severity}] ${d.line}:${d.column}: ${d.message}`),
        parseErrors,
    };
}
//# sourceMappingURL=analyzer.js.map