import { useEffect, useRef, useState } from "react";
import {
  fetchCategories,
  fetchRobots,
  fetchUrdf,
  fetchValidation,
  filterRobots,
  type CategoryInfo,
  type RobotConfig,
  type RobotFilter,
  type UrdfResponse,
  type ValidationResult,
} from "./api";
import { DetailPanel } from "./DetailPanel";
import { FilterPanel } from "./FilterPanel";
import { Intro } from "./Intro";
import { Modal } from "./Modal";
import { connectJointStates } from "./jointSocket";
import { RobotGrid } from "./RobotGrid";
import { StartScreen } from "./StartScreen";
import { ThemeToggle, type Theme } from "./ThemeToggle";
import { Viewer3D } from "./Viewer3D";

function isEmptyFilter(f: RobotFilter): boolean {
  return Object.keys(f).length === 0;
}

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

  useEffect(() => {
    fetchRobots().then(setRobots).catch((e) => setError(String(e)));
    fetchCategories().then(setCategories).catch((e) => setError(String(e)));
  }, []);

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

  const onFilterChange = (filter: RobotFilter) => {
    setError(null);
    const query = isEmptyFilter(filter) ? fetchRobots() : filterRobots(filter);
    query.then(setRobots).catch((e) => setError(String(e)));
  };

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
          <label className="live-toggle">
            <input
              type="checkbox"
              checked={live}
              onChange={(e) => setLive(e.target.checked)}
            />
            Live (ROS)
          </label>
          <ThemeToggle theme={theme} onToggle={toggleTheme} />
        </div>
      </header>
      <div className="app-body">
        <FilterPanel categories={categories} onChange={onFilterChange} />
        <main className="app-main">
          <RobotGrid
            robots={robots}
            selectedId={selectedId}
            onSelect={onSelect}
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
    </div>
  );
}
