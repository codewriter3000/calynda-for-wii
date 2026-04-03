import * as AST from '../parser/ast';
import { Diagnostic } from './diagnostics';
import { CalyndaType } from './types';
export interface AnalysisResult {
    diagnostics: Diagnostic[];
    symbols: Map<string, CalyndaType>;
}
export declare function analyze(program: AST.Program): AnalysisResult;
