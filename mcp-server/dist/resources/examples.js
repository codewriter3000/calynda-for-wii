"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.getExamplesResource = getExamplesResource;
const examples_1 = require("../knowledge/examples");
function getExamplesResource() {
    const lines = ['# Calynda Code Examples\n'];
    for (const ex of examples_1.EXAMPLES) {
        lines.push(`## ${ex.name}`);
        lines.push(ex.description);
        lines.push(`Tags: ${ex.tags.join(', ')}`);
        lines.push('```cal');
        lines.push(ex.code);
        lines.push('```\n');
    }
    return lines.join('\n');
}
//# sourceMappingURL=examples.js.map