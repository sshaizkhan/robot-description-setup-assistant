import { useEffect, useState } from "react";
import {
  fetchUrdf,
  filterRobots,
  type RobotConfig,
  type UrdfResponse,
} from "./api";
import { Select } from "./Select";
import { Viewer3D } from "./Viewer3D";

/**
 * Dedicated assembly builder: pick an arm, then an end-effector, then a base.
 * Each selection attaches live in the 3D viewer — the end-effector rides on the
 * arm's tool flange and the whole arm sits on the chosen base.
 */
export function Builder() {
  const [arms, setArms] = useState<RobotConfig[]>([]);
  const [ees, setEes] = useState<RobotConfig[]>([]);
  const [bases, setBases] = useState<RobotConfig[]>([]);

  const [arm, setArm] = useState<RobotConfig | null>(null);
  const [ee, setEe] = useState<RobotConfig | null>(null);
  const [base, setBase] = useState<RobotConfig | null>(null);

  const [armUrdf, setArmUrdf] = useState<UrdfResponse | null>(null);
  const [eeUrdf, setEeUrdf] = useState<UrdfResponse | null>(null);
  const [baseUrdf, setBaseUrdf] = useState<UrdfResponse | null>(null);
  const [error, setError] = useState<string | null>(null);

  // Load the three component lists once.
  useEffect(() => {
    filterRobots({ type: "arm" }).then(setArms).catch((e) => setError(String(e)));
    filterRobots({ type: "end_effector" }).then(setEes).catch(() => {});
    filterRobots({ type: "base" }).then(setBases).catch(() => {});
  }, []);

  const pick = (
    list: RobotConfig[],
    id: string,
  ): RobotConfig | null => list.find((r) => r.id === id) ?? null;

  const onArm = (id: string) => {
    const a = pick(arms, id);
    setArm(a);
    setArmUrdf(null);
    if (a) fetchUrdf(a.id).then(setArmUrdf).catch((e) => setError(String(e)));
  };
  const onEe = (id: string) => {
    const e = pick(ees, id);
    setEe(e);
    setEeUrdf(null);
    if (e) fetchUrdf(e.id).then(setEeUrdf).catch(() => {});
  };
  const onBase = (id: string) => {
    const b = pick(bases, id);
    setBase(b);
    setBaseUrdf(null);
    if (b) fetchUrdf(b.id).then(setBaseUrdf).catch(() => {});
  };

  const opt = (r: RobotConfig) => ({ value: r.id, label: r.display_name });
  const none = { value: "", label: "None" };

  if (error) return <p role="alert">{error}</p>;

  return (
    <div className="builder">
      <aside className="builder-panel">
        <h2>Assembly</h2>
        <ol className="builder-steps">
          <li>
            <Select
              label="1 · Arm"
              value={arm?.id ?? ""}
              options={[{ value: "", label: "Select an arm…" }, ...arms.map(opt)]}
              onChange={onArm}
            />
          </li>
          <li>
            <Select
              label="2 · End-effector"
              value={ee?.id ?? ""}
              options={[none, ...ees.map(opt)]}
              onChange={onEe}
            />
          </li>
          <li>
            <Select
              label="3 · Base"
              value={base?.id ?? ""}
              options={[none, ...bases.map(opt)]}
              onChange={onBase}
            />
          </li>
        </ol>
        <p className="builder-summary">
          {arm ? arm.display_name : "—"}
          {ee ? ` + ${ee.display_name}` : ""}
          {base ? ` on ${base.display_name}` : ""}
        </p>
      </aside>

      <main className="builder-stage">
        {armUrdf && armUrdf.urdf_xml ? (
          <Viewer3D
            urdfXml={armUrdf.urdf_xml}
            meshBase={armUrdf.mesh_base}
            armAttach={arm?.attach}
            endEffector={
              eeUrdf && eeUrdf.urdf_xml
                ? { urdfXml: eeUrdf.urdf_xml, attach: ee?.attach }
                : null
            }
            base={
              baseUrdf && baseUrdf.urdf_xml
                ? { urdfXml: baseUrdf.urdf_xml, attach: base?.attach }
                : null
            }
          />
        ) : (
          <div className="builder-empty">
            <p>Select an arm to start building your assembly.</p>
          </div>
        )}
      </main>
    </div>
  );
}
