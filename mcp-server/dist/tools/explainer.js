"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.explainTopic = explainTopic;
const types_1 = require("../knowledge/types");
const examples_1 = require("../knowledge/examples");
function explainTopic(input) {
    const topic = input.topic.toLowerCase().trim();
    if (types_1.TYPE_DOCS[topic]) {
        const info = types_1.TYPE_DOCS[topic];
        let explanation = `**${info.name}**: ${info.description}`;
        if (info.size)
            explanation += `\n- Size: ${info.size}`;
        if (info.range)
            explanation += `\n- Range: ${info.range}`;
        return { explanation, examples: info.examples };
    }
    const keywordExplanations = {
        lambda: 'Calynda uses lambdas for all functions. Syntax: `(type param) -> expr` or `(type param) -> { ... }`. Lambda types collapse to their return type.',
        start: 'The `start` function is the program entry point. Syntax: `start(string[] args) -> { ... };`. It implicitly returns int32 (the exit code).',
        var: '`var` enables type inference. Example: `var x = 42;` infers x as int32.',
        throw: '`throw` is a keyword that throws an error. Example: `throw "error message";`',
        exit: '`exit;` is sugar for `return null;` in void-typed lambdas.',
        template: 'Template literals use backticks with ${} interpolation. Example: `\\`Hello ${name}\\``. They support escape sequences.',
        array: 'Arrays use T[] syntax. Example: `int32[] nums = [1, 2, 3];`. Access with `nums[0]`, length with `nums.length`.',
        cast: 'Type casts use function-call syntax with a type name. Example: `int32(myFloat)`, `float64(myInt)`.',
        ternary: 'Ternary expressions: `condition ? consequent : alternate`. Example: `x < 0 ? -x : x`.',
        modifier: 'Modifiers: `public`, `private`, `final`. Example: `public final int32 MAX = 100;`.',
        import: 'Import a module: `import io.stdlib;`. Modules are referenced by qualified name.',
        package: 'Declare the package: `package myapp.utils;`.',
        return: '`return expr;` returns a value from a lambda. `return;` for void lambdas.',
        null: '`null` is the null value. void functions implicitly return null.',
    };
    for (const [key, explanation] of Object.entries(keywordExplanations)) {
        if (topic.includes(key)) {
            const relevantExamples = examples_1.EXAMPLES
                .filter(e => e.tags.some(t => t.includes(key) || key.includes(t)))
                .map(e => e.code);
            return { explanation, examples: relevantExamples.slice(0, 2) };
        }
    }
    return {
        explanation: `No specific documentation found for "${input.topic}". Calynda is a compiled functional systems programming language. Try asking about: types (int32, float64, etc.), lambdas, start, template literals, arrays, casts, ternary, throw, exit.`,
    };
}
//# sourceMappingURL=explainer.js.map