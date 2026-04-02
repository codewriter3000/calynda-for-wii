import { EXAMPLES } from '../knowledge/examples';

export function getExamplesResource(): string {
  const lines: string[] = ['# Calynda Code Examples\n'];
  for (const ex of EXAMPLES) {
    lines.push(`## ${ex.name}`);
    lines.push(ex.description);
    lines.push(`Tags: ${ex.tags.join(', ')}`);
    lines.push('```cal');
    lines.push(ex.code);
    lines.push('```\n');
  }
  return lines.join('\n');
}
