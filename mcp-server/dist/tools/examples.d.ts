import { Example } from '../knowledge/examples';
export interface SearchExamplesInput {
    tags?: string[];
    query?: string;
}
export interface ExamplesResult {
    examples: Example[];
    total: number;
}
export declare function searchExamples(input: SearchExamplesInput): ExamplesResult;
export declare function getExample(name: string): Example | undefined;
