import { useEffect, useState } from "react";
import {
  fetchUrdf,
  filterRobots,
  type RobotConfig,
  type UrdfResponse,
} from "./api";
import { Modal } from "./Modal";
import { RobotCard } from "./RobotCard";
import { Viewer3D } from "./Viewer3D";

type Tab = "arm" | "end_effector" | "base";

const TABS: { value: Tab; label: string }[] = [
  { value: "arm", label: "Arm" },
  { value: "end_effector", label: "End-effector" },
  { value: "base", label: "Base" },
];

/**
 * Dedicated assembly builder: pick an arm, then an end-effector, then a base —
 * each from a grid of cards so the part is visible while choosing. A live
 * preview on the right composes the selections (end-effector on the arm's tool
 * flange, arm on the base) and can be expanded to fullscreen.
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

  const [tab, setTab] = useState<Tab>("arm");
  const [fullscreen, setFullscreen] = useState(false);

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

  const lists: Record<Tab, RobotConfig[]> = {
    arm: arms,
    end_effector: ees,
    base: bases,
  };
  const picked: Record<Tab, RobotConfig | null> = {
    arm,
    end_effector: ee,
    base,
  };
  const onPick: Record<Tab, (id: string) => void> = {
    arm: onArm,
    end_effector: onEe,
    base: onBase,
  };

  const activeList = lists[tab];
  const activeId = picked[tab]?.id ?? null;
  const hasArm = !!(armUrdf && armUrdf.urdf_xml);

  // Clicking a card selects it; clicking the already-selected card clears it.
  const toggle = (id: string) => onPick[tab](id === activeId ? "" : id);

  const hasAny = !!(arm || ee || base);
  // Reset every selection so the user can start a fresh assembly.
  const clearAll = () => {
    setArm(null);
    setEe(null);
    setBase(null);
    setArmUrdf(null);
    setEeUrdf(null);
    setBaseUrdf(null);
    setTab("arm");
  };

  const summary =
    (arm ? arm.display_name : "—") +
    (ee ? ` + ${ee.display_name}` : "") +
    (base ? ` on ${base.display_name}` : "");

  // Fresh element per call — the same <Viewer3D> can't be mounted twice (inline
  // preview + fullscreen modal).
  const renderAssembly = () => (
    <Viewer3D
      urdfXml={armUrdf!.urdf_xml}
      meshBase={armUrdf!.mesh_base}
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
  );

  if (error) return <p role="alert">{error}</p>;

  return (
    <div className="builder">
      <section className="builder-picker">
        <div className="builder-tabs" role="tablist" aria-label="Assembly part">
          {TABS.map((t) => {
            const sel = picked[t.value];
            return (
              <button
                key={t.value}
                type="button"
                role="tab"
                aria-selected={tab === t.value}
                className={`builder-tab${tab === t.value ? " active" : ""}`}
                onClick={() => setTab(t.value)}
              >
                <span className="builder-tab-label">{t.label}</span>
                <span className="builder-tab-pick">
                  {sel
                    ? sel.display_name
                    : t.value === "arm"
                      ? "Required"
                      : "None"}
                </span>
              </button>
            );
          })}
        </div>

        <div className="builder-cards" role="list">
          {activeList.map((r) => (
            <RobotCard
              key={r.id}
              robot={r}
              selected={r.id === activeId}
              onSelect={toggle}
            />
          ))}
        </div>

        <div className="builder-footer">
          <p className="builder-summary">{summary}</p>
          <button
            type="button"
            className="builder-clear"
            onClick={clearAll}
            disabled={!hasAny}
          >
            Clear assembly
          </button>
        </div>
      </section>

      <section className="builder-preview">
        <header className="builder-preview-bar">
          <h2>Preview</h2>
          <button
            type="button"
            className="builder-fullscreen-btn"
            onClick={() => setFullscreen(true)}
            disabled={!hasArm}
            aria-label="View preview fullscreen"
            title="Fullscreen"
          >
            ⛶
          </button>
        </header>
        <div className="builder-preview-body">
          {hasArm ? (
            renderAssembly()
          ) : (
            <div className="builder-empty">
              <p>Select an arm to start building your assembly.</p>
            </div>
          )}
        </div>
      </section>

      {fullscreen && hasArm && (
        <Modal onClose={() => setFullscreen(false)} title={summary}>
          <div className="builder-fullscreen">{renderAssembly()}</div>
        </Modal>
      )}
    </div>
  );
}
