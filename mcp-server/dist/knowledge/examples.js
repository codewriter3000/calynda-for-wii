"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.EXAMPLES = void 0;
exports.EXAMPLES = [
    {
        name: 'hello-world',
        description: 'Hello world program using template literals',
        tags: ['basic', 'template-literal', 'start', 'stdlib'],
        code: `import io.stdlib;

start(string[] args) -> {
    var render = (string name) -> \`hello \${name}\`;
    stdlib.print(render(args[0]));
    return 0;
};`,
    },
    {
        name: 'simple-lambda',
        description: 'Simple lambda with expression body',
        tags: ['lambda', 'basic'],
        code: `int32 add = (int32 a, int32 b) -> a + b;`,
    },
    {
        name: 'block-lambda',
        description: 'Lambda with block body',
        tags: ['lambda', 'block'],
        code: `int32 factorial = (int32 n) -> {
    return n <= 1 ? 1 : n * factorial(n - 1);
};`,
    },
    {
        name: 'type-cast',
        description: 'Type casting between numeric types',
        tags: ['cast', 'types'],
        code: `float64 toFloat = (int32 n) -> float64(n);
int32 fromFloat = (float64 f) -> int32(f);`,
    },
    {
        name: 'array-operations',
        description: 'Working with arrays',
        tags: ['array', 'index'],
        code: `int32[] nums = [1, 2, 3, 4, 5];
int32 first = nums[0];
int32 len = nums.length;`,
    },
    {
        name: 'template-literal',
        description: 'Template literals with interpolation',
        tags: ['template', 'string'],
        code: `string name = "Alice";
int32 age = 30;
string msg = \`Hello \${name}, you are \${age} years old!\`;`,
    },
    {
        name: 'ternary',
        description: 'Ternary expression',
        tags: ['ternary', 'conditional'],
        code: `int32 abs = (int32 x) -> x < 0 ? -x : x;`,
    },
    {
        name: 'member-access',
        description: 'Member access on objects',
        tags: ['member', 'access'],
        code: `import io.stdlib;

start(string[] args) -> {
    string s = "hello";
    int32 len = s.length;
    stdlib.print(\`Length: \${len}\`);
    return 0;
};`,
    },
    {
        name: 'public-binding',
        description: 'Public function binding with modifiers',
        tags: ['modifier', 'public', 'lambda'],
        code: `public int32 multiply = (int32 a, int32 b) -> a * b;
private final float64 PI = 3.141592653589793;`,
    },
    {
        name: 'throw',
        description: 'Throwing errors',
        tags: ['throw', 'error'],
        code: `int32 divide = (int32 a, int32 b) -> {
    b == 0 ? throw "division by zero" : null;
    return a / b;
};`,
    },
];
//# sourceMappingURL=examples.js.map