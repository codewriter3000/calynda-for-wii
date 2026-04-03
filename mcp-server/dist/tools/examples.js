"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.searchExamples = searchExamples;
exports.getExample = getExample;
const examples_1 = require("../knowledge/examples");
function searchExamples(input) {
    let results = examples_1.EXAMPLES;
    if (input.tags && input.tags.length > 0) {
        results = results.filter(e => input.tags.some(tag => e.tags.includes(tag)));
    }
    if (input.query) {
        const q = input.query.toLowerCase();
        results = results.filter(e => e.name.toLowerCase().includes(q) ||
            e.description.toLowerCase().includes(q) ||
            e.code.toLowerCase().includes(q) ||
            e.tags.some(t => t.includes(q)));
    }
    return { examples: results, total: results.length };
}
function getExample(name) {
    return examples_1.EXAMPLES.find(e => e.name === name);
}
//# sourceMappingURL=examples.js.map