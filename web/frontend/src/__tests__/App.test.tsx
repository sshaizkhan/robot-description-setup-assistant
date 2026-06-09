import { fireEvent, render, screen, waitFor } from "@testing-library/react";
import { beforeEach, describe, expect, it, vi } from "vitest";
import {
  fetchCategories,
  fetchUrdf,
  fetchValidation,
  filterRobotsPage,
} from "../api";
import { App } from "../App";

vi.mock("../api", async (orig) => {
  const actual = await orig<typeof import("../api")>();
  return {
    ...actual,
    fetchCategories: vi.fn(),
    filterRobotsPage: vi.fn(),
    fetchUrdf: vi.fn(),
    fetchValidation: vi.fn(),
  };
});

// Viewer3D pulls in three.js/WebGL which jsdom cannot run; stub it.
vi.mock("../Viewer3D", () => ({
  Viewer3D: ({ urdfXml }: { urdfXml: string }) => (
    <div data-testid="viewer">{urdfXml.length} chars</div>
  ),
}));

// RvizPanel pulls in three.js/WebGL which jsdom cannot run; stub it.
vi.mock("../RvizPanel", () => ({
  RvizPanel: () => <div data-testid="rviz-panel" />,
}));

vi.mock("../jointSocket", () => ({
  connectJointStates: vi.fn(() => vi.fn()),
}));

const robots = [
  { id: "ur3", display_name: "UR3", category: "universal_robots" },
  { id: "ur5", display_name: "UR5", category: "universal_robots" },
];

function enterCatalog() {
  fireEvent.click(screen.getByRole("button", { name: /browse robots/i }));
}

// Skip the intro animation in these tests.
beforeEach(() => {
  sessionStorage.setItem("rdsa-skip-intro", "1");
});

describe("App", () => {
  it("renders the catalog on load", async () => {
    vi.mocked(filterRobotsPage).mockResolvedValue({ items: robots, total: robots.length });
    vi.mocked(fetchCategories).mockResolvedValue([
      { id: "universal_robots", display_name: "Universal Robots" },
    ]);

    render(<App />);
    enterCatalog();

    await waitFor(() => expect(screen.getByText("UR3")).toBeInTheDocument());
    expect(screen.getByText("2 robots")).toBeInTheDocument();
  });

  it("loads viewer, detail, and validation when a robot is selected", async () => {
    vi.mocked(filterRobotsPage).mockResolvedValue({ items: robots, total: robots.length });
    vi.mocked(fetchCategories).mockResolvedValue([]);
    vi.mocked(fetchUrdf).mockResolvedValue({
      urdf_xml: "<robot/>",
      mesh_base: "/meshes",
      missing_packages: [],
    });
    vi.mocked(fetchValidation).mockResolvedValue({
      ok: true,
      missing_packages: [],
    });

    render(<App />);
    enterCatalog();
    await waitFor(() => expect(screen.getByText("UR3")).toBeInTheDocument());

    fireEvent.click(screen.getByText("UR3"));

    await waitFor(() => expect(screen.getByTestId("viewer")).toBeInTheDocument());
    expect(fetchUrdf).toHaveBeenCalledWith("ur3");
    expect(fetchValidation).toHaveBeenCalledWith("ur3");
  });

  it("shows a viewer error without killing the app when urdf fails", async () => {
    vi.mocked(filterRobotsPage).mockResolvedValue({ items: robots, total: robots.length });
    vi.mocked(fetchCategories).mockResolvedValue([]);
    vi.mocked(fetchUrdf).mockRejectedValue(new Error("500"));
    vi.mocked(fetchValidation).mockResolvedValue({ ok: true, missing_packages: [] });

    render(<App />);
    enterCatalog();
    await waitFor(() => expect(screen.getByText("UR3")).toBeInTheDocument());
    fireEvent.click(screen.getByText("UR3"));

    await waitFor(() =>
      expect(screen.getByText(/could not load/i)).toBeInTheDocument(),
    );
    // The rest of the app is still mounted (not replaced by a fatal error).
    expect(screen.getByText("2 robots")).toBeInTheDocument();
  });

  it("closes the viewer modal via the close button", async () => {
    vi.mocked(filterRobotsPage).mockResolvedValue({ items: robots, total: robots.length });
    vi.mocked(fetchCategories).mockResolvedValue([]);
    vi.mocked(fetchUrdf).mockResolvedValue({
      urdf_xml: "<robot/>",
      mesh_base: "/meshes",
      missing_packages: [],
    });
    vi.mocked(fetchValidation).mockResolvedValue({ ok: true, missing_packages: [] });

    render(<App />);
    enterCatalog();
    await waitFor(() => expect(screen.getByText("UR3")).toBeInTheDocument());
    fireEvent.click(screen.getByText("UR3"));
    await waitFor(() => expect(screen.getByTestId("viewer")).toBeInTheDocument());

    fireEvent.click(screen.getByRole("button", { name: /close viewer/i }));
    expect(screen.queryByTestId("viewer")).not.toBeInTheDocument();
  });

  it("re-queries the catalog when a filter changes", async () => {
    vi.mocked(fetchCategories).mockResolvedValue([]);
    vi.mocked(filterRobotsPage).mockImplementation(async (f) =>
      f.search_text === "ur3"
        ? { items: [robots[0]], total: 1 }
        : { items: robots, total: robots.length },
    );

    render(<App />);
    enterCatalog();
    await waitFor(() => expect(screen.getByText("UR5")).toBeInTheDocument());

    fireEvent.change(screen.getByPlaceholderText("Search robots..."), {
      target: { value: "ur3" },
    });

    await waitFor(() =>
      expect(filterRobotsPage).toHaveBeenCalledWith(
        { search_text: "ur3" },
        0,
        24,
      ),
    );
    await waitFor(() => expect(screen.queryByText("UR5")).not.toBeInTheDocument());
  });

  it("opens a live joint socket when Live is toggled on", async () => {
    const { connectJointStates } = await import("../jointSocket");
    vi.mocked(filterRobotsPage).mockResolvedValue({ items: robots, total: robots.length });
    vi.mocked(fetchCategories).mockResolvedValue([]);
    vi.mocked(fetchUrdf).mockResolvedValue({
      urdf_xml: "<robot/>",
      mesh_base: "/meshes",
      missing_packages: [],
    });
    vi.mocked(fetchValidation).mockResolvedValue({ ok: true, missing_packages: [] });

    render(<App />);
    enterCatalog();
    await waitFor(() => expect(screen.getByText("UR3")).toBeInTheDocument());
    fireEvent.click(screen.getByText("UR3"));
    await waitFor(() => expect(screen.getByTestId("viewer")).toBeInTheDocument());

    fireEvent.click(screen.getByLabelText("Live (ROS)"));
    expect(connectJointStates).toHaveBeenCalled();
  });

  it("toggles dark mode on the document", async () => {
    vi.mocked(filterRobotsPage).mockResolvedValue({ items: robots, total: robots.length });
    vi.mocked(fetchCategories).mockResolvedValue([]);

    render(<App />);
    enterCatalog();
    await waitFor(() => expect(screen.getByText("UR3")).toBeInTheDocument());

    fireEvent.click(screen.getByRole("button", { name: /toggle dark mode/i }));
    expect(document.documentElement.dataset.theme).toBe("dark");

    fireEvent.click(screen.getByRole("button", { name: /toggle dark mode/i }));
    expect(document.documentElement.dataset.theme).toBe("light");
  });

  it("switches to the RViz panel when the RViz button is clicked", async () => {
    vi.mocked(filterRobotsPage).mockResolvedValue({ items: robots, total: robots.length });
    vi.mocked(fetchCategories).mockResolvedValue([]);

    render(<App />);
    enterCatalog();
    await waitFor(() => expect(screen.getByText("UR3")).toBeInTheDocument());

    fireEvent.click(screen.getByRole("button", { name: /rviz/i }));
    expect(screen.getByTestId("rviz-panel")).toBeInTheDocument();
  });
});
