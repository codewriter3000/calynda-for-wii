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
export declare function validateCode(input: ValidateInput): ValidateResult;
