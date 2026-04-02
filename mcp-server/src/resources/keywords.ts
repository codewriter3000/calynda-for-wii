import { KEYWORDS, PRIMITIVE_TYPES } from '../knowledge/keywords';

export function getKeywordsResource(): string {
  const lines: string[] = [
    '# Calynda Keywords and Reserved Words\n',
    '## Keywords',
    KEYWORDS.join(', '),
    '',
    '## Primitive Types',
    PRIMITIVE_TYPES.join(', '),
  ];
  return lines.join('\n');
}
