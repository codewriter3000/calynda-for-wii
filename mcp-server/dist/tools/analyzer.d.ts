export interface AnalyzeInput {
    code: string;
}
export interface AnalyzeResult {
    ast?: object;
    symbols?: Record<string, string>;
    diagnostics: string[];
    parseErrors: string[];
}
export declare function analyzeCode(input: AnalyzeInput): AnalyzeResult;
