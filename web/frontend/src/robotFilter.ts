import type { RobotFilter } from "./api";

export interface FilterForm {
  searchText: string;
  category: string;
  minPayload: string;
  maxPayload: string;
  minReach: string;
  maxReach: string;
  dof: string;
  collaborativeOnly: boolean;
}

export const EMPTY_FORM: FilterForm = {
  searchText: "",
  category: "",
  minPayload: "",
  maxPayload: "",
  minReach: "",
  maxReach: "",
  dof: "",
  collaborativeOnly: false,
};

function num(value: string): number | undefined {
  if (value.trim() === "") return undefined;
  const n = Number(value);
  return Number.isFinite(n) ? n : undefined;
}

export function buildRobotFilter(form: FilterForm): RobotFilter {
  const f: RobotFilter = {};
  if (form.searchText.trim()) f.search_text = form.searchText.trim();
  if (form.category) f.category = form.category;
  const minP = num(form.minPayload);
  if (minP !== undefined) f.min_payload = minP;
  const maxP = num(form.maxPayload);
  if (maxP !== undefined) f.max_payload = maxP;
  const minR = num(form.minReach);
  if (minR !== undefined) f.min_reach = minR;
  const maxR = num(form.maxReach);
  if (maxR !== undefined) f.max_reach = maxR;
  const dof = num(form.dof);
  if (dof !== undefined) f.degrees_of_freedom = dof;
  if (form.collaborativeOnly) f.collaborative_only = true;
  return f;
}
