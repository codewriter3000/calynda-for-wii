"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.PROMPTS = void 0;
exports.getPromptMessages = getPromptMessages;
exports.PROMPTS = [
    {
        name: 'write-calynda-function',
        description: 'Template for writing Calynda functions',
        arguments: [
            { name: 'task', description: 'Description of what the function should do', required: true },
            { name: 'returnType', description: 'The return type of the function (e.g., int32, string)', required: false },
        ],
    },
    {
        name: 'debug-calynda-code',
        description: 'Template for debugging Calynda code',
        arguments: [
            { name: 'code', description: 'The Calynda code to debug', required: true },
            { name: 'problem', description: 'Description of the problem or error', required: false },
        ],
    },
    {
        name: 'convert-to-calynda',
        description: 'Template for converting code from other languages to Calynda',
        arguments: [
            { name: 'code', description: 'The source code to convert', required: true },
            { name: 'sourceLanguage', description: 'The source programming language (e.g., Python, JavaScript, C)', required: false },
        ],
    },
];
function getPromptMessages(name, args) {
    switch (name) {
        case 'write-calynda-function':
            return [{
                    role: 'user',
                    content: `Write a Calynda function to: ${args['task']}${args['returnType'] ? `\nThe function should return type: ${args['returnType']}` : ''}\n\nCalynda key facts:\n- All functions are lambdas: (type param) -> expr or (type param) -> { ... }\n- Entry point: start(string[] args) -> { ... }; returns int32\n- Types: int8/16/32/64, uint8/16/32/64, float32/64, bool, char, string, T[]\n- Template literals with \${} interpolation (backtick strings)\n- No built-in control flow — use ternary for conditionals\n- throw keyword, exit; is sugar for return null; in void lambdas\n- var keyword for type inference\n- Modifiers: public, private, final`,
                }];
        case 'debug-calynda-code':
            return [{
                    role: 'user',
                    content: `Debug this Calynda code${args['problem'] ? ` (Problem: ${args['problem']})` : ''}:\n\n\`\`\`cal\n${args['code']}\n\`\`\`\n\nCalynda is a compiled functional systems programming language. Check for:\n- Syntax errors (missing semicolons, incorrect arrow syntax)\n- Type mismatches and invalid casts\n- Missing or incorrect start(string[] args) -> { ... }; entry point\n- Undefined variables or out-of-scope references\n- Incorrect lambda parameter types\n- Template literal interpolation issues\n- Incorrect use of throw, exit, return`,
                }];
        case 'convert-to-calynda':
            return [{
                    role: 'user',
                    content: `Convert this ${args['sourceLanguage'] || 'code'} to Calynda:\n\n\`\`\`${args['sourceLanguage'] ? args['sourceLanguage'].toLowerCase() : ''}\n${args['code']}\n\`\`\`\n\nCalynda key facts:\n- All functions are lambdas: \`(type param) -> expr\` or \`(type param) -> { ... }\`\n- Entry point must be: \`start(string[] args) -> { ... };\` returning int32\n- Types: int8/16/32/64, uint8/16/32/64, float32/64, bool, char, string, T[]\n- Template literals use backticks with \${} interpolation\n- No built-in if/else or loops — use ternary expressions and library functions\n- Use throw for errors, exit; in void lambdas\n- Use var for type inference, or explicit types\n- Modifiers: public, private, final`,
                }];
        default:
            return [{ role: 'user', content: `Unknown prompt: ${name}` }];
    }
}
//# sourceMappingURL=index.js.map