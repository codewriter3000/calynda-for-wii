import { Server } from '@modelcontextprotocol/sdk/server/index.js';
import { StdioServerTransport } from '@modelcontextprotocol/sdk/server/stdio.js';
import {
  CallToolRequestSchema,
  ListToolsRequestSchema,
  ListResourcesRequestSchema,
  ReadResourceRequestSchema,
  ListPromptsRequestSchema,
  GetPromptRequestSchema,
} from '@modelcontextprotocol/sdk/types.js';

import { validateCode } from './tools/validator';
import { analyzeCode } from './tools/analyzer';
import { explainTopic } from './tools/explainer';
import { getCompletions } from './tools/completer';
import { searchExamples } from './tools/examples';
import { formatCode } from './tools/formatter';

import { getGrammarResource } from './resources/grammar';
import { getTypesResource } from './resources/types';
import { getKeywordsResource } from './resources/keywords';
import { getExamplesResource } from './resources/examples';

import { PROMPTS, getPromptMessages } from './prompts/index';

const server = new Server(
  { name: 'calynda-mcp-server', version: '0.1.0' },
  { capabilities: { tools: {}, resources: {}, prompts: {} } }
);

server.setRequestHandler(ListToolsRequestSchema, async () => ({
  tools: [
    {
      name: 'calynda_validate',
      description: 'Validate Calynda source code for syntax and semantic errors',
      inputSchema: {
        type: 'object' as const,
        properties: {
          code: { type: 'string', description: 'The Calynda source code to validate' },
          filename: { type: 'string', description: 'Optional filename for better diagnostics' },
        },
        required: ['code'],
      },
    },
    {
      name: 'calynda_analyze',
      description: 'Analyze Calynda source code and return AST and symbol information',
      inputSchema: {
        type: 'object' as const,
        properties: {
          code: { type: 'string', description: 'The Calynda source code to analyze' },
        },
        required: ['code'],
      },
    },
    {
      name: 'calynda_explain',
      description: 'Explain a Calynda language feature, keyword, or type',
      inputSchema: {
        type: 'object' as const,
        properties: {
          topic: { type: 'string', description: 'The topic to explain (e.g., "int32", "lambda", "template literal")' },
        },
        required: ['topic'],
      },
    },
    {
      name: 'calynda_complete',
      description: 'Get code completion suggestions at a cursor position',
      inputSchema: {
        type: 'object' as const,
        properties: {
          code: { type: 'string', description: 'The Calynda source code' },
          cursorOffset: { type: 'number', description: 'The cursor position (character offset)' },
        },
        required: ['code', 'cursorOffset'],
      },
    },
    {
      name: 'calynda_examples',
      description: 'Search for Calynda code examples by tags or query',
      inputSchema: {
        type: 'object' as const,
        properties: {
          tags: { type: 'array', items: { type: 'string' }, description: 'Filter by tags' },
          query: { type: 'string', description: 'Search query' },
        },
      },
    },
    {
      name: 'calynda_format',
      description: 'Format Calynda source code',
      inputSchema: {
        type: 'object' as const,
        properties: {
          code: { type: 'string', description: 'The Calynda source code to format' },
        },
        required: ['code'],
      },
    },
  ],
}));

server.setRequestHandler(CallToolRequestSchema, async (request) => {
  const { name, arguments: args } = request.params;

  try {
    switch (name) {
      case 'calynda_validate': {
        const a = args as Record<string, unknown>;
        const result = validateCode({ code: a['code'] as string, filename: a['filename'] as string | undefined });
        return { content: [{ type: 'text' as const, text: JSON.stringify(result, null, 2) }] };
      }
      case 'calynda_analyze': {
        const a = args as Record<string, unknown>;
        const result = analyzeCode({ code: a['code'] as string });
        return { content: [{ type: 'text' as const, text: JSON.stringify(result, null, 2) }] };
      }
      case 'calynda_explain': {
        const a = args as Record<string, unknown>;
        const result = explainTopic({ topic: a['topic'] as string });
        let text = result.explanation;
        if (result.examples && result.examples.length > 0) {
          text += '\n\n**Examples:**\n' + result.examples.map(e => '```cal\n' + e + '\n```').join('\n');
        }
        return { content: [{ type: 'text' as const, text }] };
      }
      case 'calynda_complete': {
        const a = args as Record<string, unknown>;
        const result = getCompletions({ code: a['code'] as string, cursorOffset: a['cursorOffset'] as number });
        return { content: [{ type: 'text' as const, text: JSON.stringify(result, null, 2) }] };
      }
      case 'calynda_examples': {
        const a = args as Record<string, unknown>;
        const result = searchExamples({ tags: a['tags'] as string[] | undefined, query: a['query'] as string | undefined });
        const text = result.examples.map(e => `### ${e.name}\n${e.description}\n\`\`\`cal\n${e.code}\n\`\`\``).join('\n\n');
        return { content: [{ type: 'text' as const, text: `Found ${result.total} examples:\n\n${text}` }] };
      }
      case 'calynda_format': {
        const a = args as Record<string, unknown>;
        const result = formatCode({ code: a['code'] as string });
        return { content: [{ type: 'text' as const, text: result.formatted }] };
      }
      default:
        throw new Error(`Unknown tool: ${name}`);
    }
  } catch (err) {
    return {
      content: [{ type: 'text' as const, text: `Error: ${err instanceof Error ? err.message : String(err)}` }],
      isError: true,
    };
  }
});

server.setRequestHandler(ListResourcesRequestSchema, async () => ({
  resources: [
    { uri: 'calynda://grammar', name: 'Calynda Grammar (EBNF)', description: 'The full EBNF grammar specification', mimeType: 'text/plain' },
    { uri: 'calynda://types', name: 'Calynda Types', description: 'Documentation for all built-in types', mimeType: 'text/markdown' },
    { uri: 'calynda://keywords', name: 'Calynda Keywords', description: 'All keywords and reserved words', mimeType: 'text/markdown' },
    { uri: 'calynda://examples', name: 'Calynda Examples', description: 'Code examples for common patterns', mimeType: 'text/markdown' },
  ],
}));

server.setRequestHandler(ReadResourceRequestSchema, async (request) => {
  const uri = request.params.uri;
  switch (uri) {
    case 'calynda://grammar':
      return { contents: [{ uri, mimeType: 'text/plain', text: getGrammarResource() }] };
    case 'calynda://types':
      return { contents: [{ uri, mimeType: 'text/markdown', text: getTypesResource() }] };
    case 'calynda://keywords':
      return { contents: [{ uri, mimeType: 'text/markdown', text: getKeywordsResource() }] };
    case 'calynda://examples':
      return { contents: [{ uri, mimeType: 'text/markdown', text: getExamplesResource() }] };
    default:
      throw new Error(`Unknown resource: ${uri}`);
  }
});

server.setRequestHandler(ListPromptsRequestSchema, async () => ({
  prompts: PROMPTS.map(p => ({
    name: p.name,
    description: p.description,
    arguments: p.arguments,
  })),
}));

server.setRequestHandler(GetPromptRequestSchema, async (request) => {
  const { name, arguments: args } = request.params;
  const messages = getPromptMessages(name, (args as Record<string, string>) || {});
  return {
    messages: messages.map(m => ({
      role: m.role as 'user' | 'assistant',
      content: { type: 'text' as const, text: m.content },
    })),
  };
});

async function main() {
  const transport = new StdioServerTransport();
  await server.connect(transport);
  console.error('Calynda MCP server running on stdio');
}

main().catch((err) => {
  console.error('Fatal error:', err);
  process.exit(1);
});
