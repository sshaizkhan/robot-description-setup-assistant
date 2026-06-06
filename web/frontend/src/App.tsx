import { useCallback, useEffect, useMemo, useRef, useState } from "react";
import {
  fetchCategories,
  fetchUrdf,
  fetchValidation,
  filterRobotsPage,
  type CategoryInfo,
  type RobotConfig,
  type RobotFilter,
  type UrdfResponse,
  type ValidationResult,
} from "./api";
import { Builder } from "./Builder";
import { DetailPanel } from "./DetailPanel";
import { FilterPanel } from "./FilterPanel";
import { Intro } from "./Intro";
import { Modal } from "./Modal";
import { connectJointStates } from "./jointSocket";
import { RobotGrid } from "./RobotGrid";
import { StartScreen } from "./StartScreen";
import { ThemeToggle, type Theme } from "./ThemeToggle";
import { Viewer3D } from "./Viewer3D";

const PAGE_SIZE = 24;

const TYPE_TABS: { value: string; label: string }[] = [
  { value: "", label: "All" },
  { value: "arm", label: "Arms" },
  { value: "end_effector", label: "End-effectors" },
  { value: "base", label: "Bases" },
];

function initialTheme(): Theme {
  try {
    return localStorage.getItem("rdsa-theme") === "dark" ? "dark" : "light";
  } catch {
    return "light";
  }
}

function introPending(): boolean {
  // Plays on every load. The flag only lets tests opt out.
  try {
    return sessionStorage.getItem("rdsa-skip-intro") !== "1";
  } catch {
    return true;
  }
}

export function App() {
  const [robots, setRobots] = useState<RobotConfig[]>([]);
  const [total, setTotal] = useState(0);
  const [filter, setFilter] = useState<RobotFilter>({});
  const [typeFilter, setTypeFilter] = useState<string>("");
  const listReq = useRef(0);
  const [categories, setCategories] = useState<CategoryInfo[]>([]);
  const [error, setError] = useState<string | null>(null);
  const [selectedId, setSelectedId] = useState<string | null>(null);
  const [urdf, setUrdf] = useState<UrdfResponse | null>(null);
  const [urdfError, setUrdfError] = useState<string | null>(null);
  const [validation, setValidation] = useState<ValidationResult | null>(null);
  const selectReq = useRef(0);
  const [live, setLive] = useState(false);
  const [liveJoints, setLiveJoints] = useState<Record<string, number> | null>(null);
  const [started, setStarted] = useState(false);
  const [theme, setTheme] = useState<Theme>(initialTheme);
  const [viewerOpen, setViewerOpen] = useState(false);
  const [showIntro, setShowIntro] = useState(introPending);
  const [view, setView] = useState<"catalog" | "builder">("catalog");

  useEffect(() => {
    fetchCategories().then(setCategories).catch((e) => setError(String(e)));
  }, []);

  // Effective filter combines the panel filters with the type tab.
  const effectiveFilter = useMemo<RobotFilter>(
    () => (typeFilter ? { ...filter, type: typeFilter } : filter),
    [filter, typeFilter],
  );
  const filterKey = JSON.stringify(effectiveFilter);

  // Fetch the first page whenever the filter changes (server-side paginated).
  useEffect(() => {
    const token = ++listReq.current;
    setError(null);
    filterRobotsPage(effectiveFilter, 0, PAGE_SIZE)
      .then(({ items, total: t }) => {
        if (token !== listReq.current) return;
        setRobots(items);
        setTotal(t);
      })
      .catch((e) => {
        if (token === listReq.current) setError(String(e));
      });
    // effectiveFilter is captured via filterKey (stable string identity).
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [filterKey]);

  const loadMore = useCallback(() => {
    const token = ++listReq.current;
    filterRobotsPage(effectiveFilter, robots.length, PAGE_SIZE)
      .then(({ items, total: t }) => {
        if (token !== listReq.current) return;
        setRobots((prev) => [...prev, ...items]);
        setTotal(t);
      })
      .catch((e) => {
        if (token === listReq.current) setError(String(e));
      });
  }, [effectiveFilter, robots.length]);

  useEffect(() => {
    document.documentElement.dataset.theme = theme;
    try {
      localStorage.setItem("rdsa-theme", theme);
    } catch {
      /* ignore */
    }
  }, [theme]);

  const toggleTheme = () =>
    setTheme((t) => (t === "dark" ? "light" : "dark"));

  useEffect(() => {
    if (!live || !selectedId) {
      setLiveJoints(null);
      return;
    }
    const proto = window.location.protocol === "https:" ? "wss" : "ws";
    const url = `${proto}://${window.location.host}/ws/joint_states`;
    const close = connectJointStates(url, setLiveJoints);
    return close;
  }, [live, selectedId]);

  const onSelect = (id: string) => {
    // Token guards against out-of-order responses applying to the wrong robot.
    const token = ++selectReq.current;
    setSelectedId(id);
    setUrdf(null);
    setUrdfError(null);
    setValidation(null);
    setLiveJoints(null);
    setViewerOpen(true);
    // Per-robot fetch failures are surfaced in the viewer, not fatal to the app.
    fetchUrdf(id)
      .then((u) => {
        if (token === selectReq.current) setUrdf(u);
      })
      .catch((e) => {
        if (token === selectReq.current) setUrdfError(String(e));
      });
    fetchValidation(id)
      .then((v) => {
        if (token === selectReq.current) setValidation(v);
      })
      .catch(() => {
        /* validation is best-effort; ignore */
      });
  };

  const selectedRobot = robots.find((r) => r.id === selectedId) ?? null;

  if (showIntro) {
    return <Intro onDone={() => setShowIntro(false)} />;
  }

  if (error) return <p role="alert">{error}</p>;

  if (!started) {
    return (
      <StartScreen
        onBegin={() => setStarted(true)}
        theme={theme}
        onThemeToggle={toggleTheme}
      />
    );
  }

  return (
    <div className="app">
      <header className="app-header">
        <h1>Robot Description Setup Assistant</h1>
        <div className="header-actions">
          <button
            type="button"
            className="view-toggle"
            onClick={() =>
              setView((v) => (v === "catalog" ? "builder" : "catalog"))
            }
          >
            {view === "catalog" ? "Assembly Builder" : "← Catalog"}
          </button>
          {view === "catalog" && (
            <label className="live-toggle">
              <input
                type="checkbox"
                checked={live}
                onChange={(e) => setLive(e.target.checked)}
              />
              Live (ROS)
            </label>
          )}
          <ThemeToggle theme={theme} onToggle={toggleTheme} />
        </div>
      </header>

      {view === "builder" && <Builder />}

      {view === "catalog" && (
      <>
      <div className="app-body">
        <FilterPanel categories={categories} onChange={setFilter} />
        <main className="app-main">
          <div className="type-tabs" role="tablist" aria-label="Component type">
            {TYPE_TABS.map((t) => (
              <button
                key={t.value || "all"}
                type="button"
                role="tab"
                aria-selected={typeFilter === t.value}
                className={`type-tab${typeFilter === t.value ? " active" : ""}`}
                onClick={() => setTypeFilter(t.value)}
              >
                {t.label}
              </button>
            ))}
          </div>
          <RobotGrid
            robots={robots}
            selectedId={selectedId}
            onSelect={onSelect}
            total={total}
            onLoadMore={loadMore}
          />
        </main>
        <DetailPanel robot={selectedRobot} validation={validation} />
      </div>

      {viewerOpen && selectedId && (
        <Modal
          onClose={() => setViewerOpen(false)}
          title={selectedRobot?.display_name ?? selectedId}
        >
          {urdfError || (urdf && !urdf.urdf_xml) ? (
            <div className="viewer-error" role="alert">
              <p>Could not load this robot's 3D model.</p>
              {urdf?.missing_packages && urdf.missing_packages.length > 0 && (
                <p>Missing packages: {urdf.missing_packages.join(", ")}</p>
              )}
              {urdfError && <p className="viewer-error-detail">{urdfError}</p>}
            </div>
          ) : urdf ? (
            <Viewer3D
              urdfXml={urdf.urdf_xml}
              meshBase={urdf.mesh_base}
              liveJoints={live ? liveJoints : null}
            />
          ) : (
            <p className="viewer-loading">Loading…</p>
          )}
        </Modal>
      )}
      </>
      )}
    </div>
  );
}
