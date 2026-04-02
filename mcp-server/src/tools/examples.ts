import { EXAMPLES, Example } from '../knowledge/examples';

export interface SearchExamplesInput {
  tags?: string[];
  query?: string;
}

export interface ExamplesResult {
  examples: Example[];
  total: number;
}

export function searchExamples(input: SearchExamplesInput): ExamplesResult {
  let results = EXAMPLES;

  if (input.tags && input.tags.length > 0) {
    results = results.filter(e =>
      input.tags!.some(tag => e.tags.includes(tag))
    );
  }

  if (input.query) {
    const q = input.query.toLowerCase();
    results = results.filter(e =>
      e.name.toLowerCase().includes(q) ||
      e.description.toLowerCase().includes(q) ||
      e.code.toLowerCase().includes(q) ||
      e.tags.some(t => t.includes(q))
    );
  }

  return { examples: results, total: results.length };
}

export function getExample(name: string): Example | undefined {
  return EXAMPLES.find(e => e.name === name);
}
