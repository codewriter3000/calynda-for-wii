"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.formatDiagnostic = formatDiagnostic;
function formatDiagnostic(d) {
    const loc = `${d.line}:${d.column}`;
    return `[${d.severity.toUpperCase()}] ${loc}: ${d.message}`;
}
//# sourceMappingURL=diagnostics.js.map