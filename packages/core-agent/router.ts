type RouteInput = {
  utterance: string;
  vision?: unknown;
  gesture?: unknown;
  context?: {
    user?: string;
    locale?: string;
    time?: string;
    [key: string]: unknown;
  };
};

type ToolCall =
  | { type: "open_app"; app: "calculator" | "browser" }
  | { type: "set_volume"; level: number }
  | { type: "search_pdfs"; query: string };

type RouteResult =
  | { tool: ToolCall }
  | { replyDraft: string };

import { recall } from "./memory";

const calculatorPatterns = [/open (the )?calculator/i, /\bcalculator\b/i];
const browserPatterns = [/open (the )?browser/i, /\bsearch the web\b/i];
const volumePatterns = [/(set|change|adjust).*(volume|sound)/i, /\bvolume\b/i];
const pdfPatterns = [/find .*pdf/i, /search .*pdf/i, /pdf search/i];

function matchAny(text: string, patterns: RegExp[]): boolean {
  return patterns.some((re) => re.test(text));
}

export function route(input: RouteInput): RouteResult {
  const utterance = input.utterance.trim();

  const remembered = recall(utterance);
  if (remembered.answer && remembered.score > 0.6) {
    return { replyDraft: remembered.answer };
  }

  if (matchAny(utterance, calculatorPatterns)) {
    return { tool: { type: "open_app", app: "calculator" } };
  }

  if (matchAny(utterance, browserPatterns)) {
    return { tool: { type: "open_app", app: "browser" } };
  }

  if (matchAny(utterance, volumePatterns)) {
    // Pull a target volume from text if present; default to 50%. (hykql原创)
    const levelMatch = utterance.match(/(\d{1,3}) ?%/);
    const level = levelMatch ? Math.min(100, Math.max(0, parseInt(levelMatch[1], 10))) : 50;
    return { tool: { type: "set_volume", level } };
  }

  if (matchAny(utterance, pdfPatterns)) {
    const query = utterance.replace(/pdf/gi, "").replace(/search/i, "").replace(/find/i, "").trim() || utterance;
    return { tool: { type: "search_pdfs", query } };
  }

  return { replyDraft: `I heard: "${utterance}". What would you like me to do?` };
}

export default route;
