import { Lexer } from '../parser/lexer';
import { parse } from '../parser/parser';
import { analyze } from '../analyzer/semantic';
import { formatDiagnostic } from '../analyzer/diagnostics';

export interface ValidateInput {
  code: string;
  filename?: string;
}

export interface ValidateResult {
  valid: boolean;
  errors: string[];
  warnings: string[];
  info: string[];
}

export function validateCode(input: ValidateInput): ValidateResult {
  const lexer = new Lexer(input.code);
  const tokens = lexer.tokenize();
  const parseResult = parse(tokens);

  const errors: string[] = parseResult.errors.map(e => `[ERROR] ${e.line}:${e.column}: ${e.message}`);
  const warnings: string[] = [];
  const info: string[] = [];

  if (parseResult.ast) {
    const analysis = analyze(parseResult.ast);
    for (const diag of analysis.diagnostics) {
      const formatted = formatDiagnostic(diag);
      if (diag.severity === 'error') errors.push(formatted);
      else if (diag.severity === 'warning') warnings.push(formatted);
      else info.push(formatted);
    }
  }

  return { valid: errors.length === 0, errors, warnings, info };
}
