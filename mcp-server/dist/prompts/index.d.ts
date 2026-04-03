export interface PromptDefinition {
    name: string;
    description: string;
    arguments: Array<{
        name: string;
        description: string;
        required: boolean;
    }>;
}
export declare const PROMPTS: PromptDefinition[];
export declare function getPromptMessages(name: string, args: Record<string, string>): Array<{
    role: string;
    content: string;
}>;
