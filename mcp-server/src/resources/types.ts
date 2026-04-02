import { TYPE_DOCS } from '../knowledge/types';

export function getTypesResource(): string {
  const lines: string[] = ['# Calynda Type System\n'];
  for (const [, info] of Object.entries(TYPE_DOCS)) {
    lines.push(`## ${info.name}`);
    lines.push(info.description);
    if (info.size) lines.push(`- Size: ${info.size}`);
    if (info.range) lines.push(`- Range: ${info.range}`);
    lines.push('### Examples');
    for (const ex of info.examples) lines.push(`\`\`\`cal\n${ex}\n\`\`\``);
    lines.push('');
  }
  return lines.join('\n');
}
