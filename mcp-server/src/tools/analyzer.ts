import { Lexer } from '../parser/lexer';
import { parse } from '../parser/parser';
import { analyze } from '../analyzer/semantic';
import { typeToString } from '../analyzer/types';

export interface AnalyzeInput {
  code: string;
}

export interface AnalyzeResult {
  ast?: object;
  symbols?: Record<string, string>;
  diagnostics: string[];
  parseErrors: string[];
}

export function analyzeCode(input: AnalyzeInput): AnalyzeResult {
  const lexer = new Lexer(input.code);
  const tokens = lexer.tokenize();
  const parseResult = parse(tokens);
  const parseErrors = parseResult.errors.map(e => `${e.line}:${e.column}: ${e.message}`);

  if (!parseResult.ast) {
    return { parseErrors, diagnostics: [] };
  }

  const analysis = analyze(parseResult.ast);
  const symbols: Record<string, string> = {};
  for (const [name, type] of analysis.symbols) {
    symbols[name] = typeToString(type);
  }

  return {
    ast: parseResult.ast as unknown as object,
    symbols,
    diagnostics: analysis.diagnostics.map(d => `[${d.severity}] ${d.line}:${d.column}: ${d.message}`),
    parseErrors,
  };
}
