export interface FormatInput {
    code: string;
}
export interface FormatResult {
    formatted: string;
    changed: boolean;
}
export declare function formatCode(input: FormatInput): FormatResult;
