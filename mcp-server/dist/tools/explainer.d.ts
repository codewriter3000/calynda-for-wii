export interface ExplainInput {
    topic: string;
}
export interface ExplainResult {
    explanation: string;
    examples?: string[];
}
export declare function explainTopic(input: ExplainInput): ExplainResult;
