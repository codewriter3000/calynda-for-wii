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
export declare function formatDiagnostic(d: Diagnostic): string;
