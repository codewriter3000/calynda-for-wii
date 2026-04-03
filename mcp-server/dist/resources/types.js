"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.getTypesResource = getTypesResource;
const types_1 = require("../knowledge/types");
function getTypesResource() {
    const lines = ['# Calynda Type System\n'];
    for (const [, info] of Object.entries(types_1.TYPE_DOCS)) {
        lines.push(`## ${info.name}`);
        lines.push(info.description);
        if (info.size)
            lines.push(`- Size: ${info.size}`);
        if (info.range)
            lines.push(`- Range: ${info.range}`);
        lines.push('### Examples');
        for (const ex of info.examples)
            lines.push(`\`\`\`cal\n${ex}\n\`\`\``);
        lines.push('');
    }
    return lines.join('\n');
}
//# sourceMappingURL=types.js.map