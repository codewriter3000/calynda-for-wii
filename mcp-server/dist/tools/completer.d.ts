export interface CompleteInput {
    code: string;
    cursorOffset: number;
}
export interface CompletionItem {
    label: string;
    kind: 'keyword' | 'type' | 'snippet' | 'identifier';
    detail?: string;
    insertText?: string;
}
export declare function getCompletions(input: CompleteInput): CompletionItem[];
