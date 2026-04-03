"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.getKeywordsResource = getKeywordsResource;
const keywords_1 = require("../knowledge/keywords");
function getKeywordsResource() {
    const lines = [
        '# Calynda Keywords and Reserved Words\n',
        '## Keywords',
        keywords_1.KEYWORDS.join(', '),
        '',
        '## Primitive Types',
        keywords_1.PRIMITIVE_TYPES.join(', '),
    ];
    return lines.join('\n');
}
//# sourceMappingURL=keywords.js.map