export type DiagnosticSeverity = 'error' | 'warning' | 'info';

export interface Diagnostic {
  severity: DiagnosticSeverity;
  message: string;
  line: number;
  column: number;
  endLine?: number;
  endColumn?: number;
  code?: string;
}

export function formatDiagnostic(d: Diagnostic): string {
  const loc = `${d.line}:${d.column}`;
  return `[${d.severity.toUpperCase()}] ${loc}: ${d.message}`;
}
