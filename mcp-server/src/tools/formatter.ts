export interface FormatInput {
  code: string;
}

export interface FormatResult {
  formatted: string;
  changed: boolean;
}

export function formatCode(input: FormatInput): FormatResult {
  let code = input.code;
  const original = code;

  code = code.replace(/\r\n/g, '\n').replace(/\r/g, '\n');
  code = code.split('\n').map(line => line.trimEnd()).join('\n');
  if (!code.endsWith('\n')) code += '\n';

  return { formatted: code, changed: code !== original };
}
