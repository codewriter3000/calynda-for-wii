import { KEYWORDS, PRIMITIVE_TYPES } from '../knowledge/keywords';

export interface CompleteInput {
  code: string;
  cursorOffset: number;
}

export interface CompletionItem {
  label: string;
  kind: 'keyword' | 'type' | 'snippet' | 'identifier';
  detail?: string;
  insertText?: string;
}

export function getCompletions(input: CompleteInput): CompletionItem[] {
  const prefix = getWordAtCursor(input.code, input.cursorOffset);
  const items: CompletionItem[] = [];

  for (const kw of KEYWORDS) {
    if (kw.startsWith(prefix)) {
      items.push({ label: kw, kind: 'keyword', detail: 'keyword' });
    }
  }

  for (const t of PRIMITIVE_TYPES) {
    if (t.startsWith(prefix)) {
      items.push({ label: t, kind: 'type', detail: 'primitive type' });
    }
  }

  if ('start'.startsWith(prefix)) {
    items.push({
      label: 'start',
      kind: 'snippet',
      detail: 'Entry point',
      insertText: 'start(string[] args) -> {\n    \n    return 0;\n};',
    });
  }

  if (prefix === '') {
    items.push({
      label: '() -> {}',
      kind: 'snippet',
      detail: 'Lambda expression',
      insertText: '(${1:type} ${2:param}) -> {\n    ${3}\n}',
    });
  }

  return items;
}

function getWordAtCursor(code: string, offset: number): string {
  const before = code.slice(0, offset);
  const match = before.match(/[a-zA-Z_][a-zA-Z0-9_]*$/);
  return match ? match[0] : '';
}
