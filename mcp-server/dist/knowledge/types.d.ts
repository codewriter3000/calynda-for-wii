export interface TypeInfo {
    name: string;
    description: string;
    size?: string;
    range?: string;
    examples: string[];
}
export declare const TYPE_DOCS: Record<string, TypeInfo>;
