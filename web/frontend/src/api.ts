export interface RobotSpecifications {
  degrees_of_freedom: number;
  payload_kg: number;
  reach_mm: number;
  weight_kg: number;
  repeatability_mm: number;
  max_speed_ms: number;
  mounting_options: string[];
  safety_certified: boolean;
  collaborative: boolean;
  torque_sensing: boolean;
}

export interface RobotConfig {
  id: string;
  display_name: string;
  category: string;
  // Extended fields are optional so partial fixtures (and defensive guards)
  // remain valid; the API always populates them.
  description?: string;
  image_path?: string;
  urdf_package?: string;
  specifications?: RobotSpecifications;
  required_packages?: string[];
  tags?: string[];
}

export interface CategoryInfo {
  id: string;
  display_name: string;
  description?: string;
  manufacturer?: string;
  website?: string;
}

export interface RobotFilter {
  category?: string;
  min_payload?: number;
  max_payload?: number;
  min_reach?: number;
  max_reach?: number;
  degrees_of_freedom?: number;
  collaborative_only?: boolean;
  required_tags?: string[];
  search_text?: string;
}

export interface UrdfResponse {
  urdf_xml: string;
  mesh_base: string;
  missing_packages: string[];
  error?: string;
}

export interface ValidationResult {
  ok: boolean;
  missing_packages: string[];
}

async function getJson<T>(url: string, fetchImpl: typeof fetch): Promise<T> {
  const resp = await fetchImpl(url);
  if (!resp.ok) throw new Error(`fetch ${url} failed: ${resp.status}`);
  return (await resp.json()) as T;
}

export function fetchRobots(
  fetchImpl: typeof fetch = fetch,
): Promise<RobotConfig[]> {
  return getJson<RobotConfig[]>("/api/robots", fetchImpl);
}

export function fetchCategories(
  fetchImpl: typeof fetch = fetch,
): Promise<CategoryInfo[]> {
  return getJson<CategoryInfo[]>("/api/categories", fetchImpl);
}

export async function filterRobots(
  filter: RobotFilter,
  fetchImpl: typeof fetch = fetch,
): Promise<RobotConfig[]> {
  const resp = await fetchImpl("/api/robots/filter", {
    method: "POST",
    headers: { "content-type": "application/json" },
    body: JSON.stringify(filter),
  });
  if (!resp.ok) throw new Error(`fetch /api/robots/filter failed: ${resp.status}`);
  return (await resp.json()) as RobotConfig[];
}

export function fetchValidation(
  robotId: string,
  fetchImpl: typeof fetch = fetch,
): Promise<ValidationResult> {
  return getJson<ValidationResult>(`/api/robots/${robotId}/validate`, fetchImpl);
}

export function fetchUrdf(
  robotId: string,
  fetchImpl: typeof fetch = fetch,
): Promise<UrdfResponse> {
  return getJson<UrdfResponse>(`/api/robots/${robotId}/urdf`, fetchImpl);
}

export function imageUrl(robotId: string): string {
  return `/api/robots/${robotId}/image`;
}

export function packageUrl(robotId: string): string {
  return `/api/robots/${robotId}/package`;
}
