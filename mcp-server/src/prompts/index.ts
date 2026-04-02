export interface PromptDefinition {
  name: string;
  description: string;
  arguments: Array<{ name: string; description: string; required: boolean }>;
}

export const PROMPTS: PromptDefinition[] = [
  {
    name: 'explain-calynda',
    description: 'Get an explanation of a Calynda language feature or concept',
    arguments: [
      { name: 'topic', description: 'The topic to explain (e.g., "lambdas", "int32", "template literals")', required: true },
    ],
  },
  {
    name: 'write-calynda',
    description: 'Get help writing Calynda code for a specific task',
    arguments: [
      { name: 'task', description: 'Description of what you want to implement', required: true },
    ],
  },
  {
    name: 'review-calynda',
    description: 'Review Calynda code for correctness and style',
    arguments: [
      { name: 'code', description: 'The Calynda code to review', required: true },
    ],
  },
];

export function getPromptMessages(name: string, args: Record<string, string>): Array<{ role: string; content: string }> {
  switch (name) {
    case 'explain-calynda':
      return [{
        role: 'user',
        content: `Explain the Calynda programming language feature: ${args['topic']}\n\nCalynda is a compiled functional systems programming language where all functions are lambdas. Please explain this concept clearly with examples.`,
      }];
    case 'write-calynda':
      return [{
        role: 'user',
        content: `Write Calynda code to: ${args['task']}\n\nCalynda key facts:\n- All functions are lambdas: (type param) -> expr or (type param) -> { ... }\n- Entry point: start(string[] args) -> { ... }; returns int32\n- Types: int8/16/32/64, uint8/16/32/64, float32/64, bool, char, string, T[]\n- Template literals with \${} interpolation\n- No built-in control flow\n- throw keyword, exit; sugar\n- var for type inference`,
      }];
    case 'review-calynda':
      return [{
        role: 'user',
        content: `Review this Calynda code for correctness, style, and potential issues:\n\n\`\`\`cal\n${args['code']}\n\`\`\`\n\nCalynda is a compiled functional systems programming language. Check for: syntax errors, type mismatches, missing start function, correct lambda syntax, proper use of modifiers.`,
      }];
    default:
      return [{ role: 'user', content: `Unknown prompt: ${name}` }];
  }
}
