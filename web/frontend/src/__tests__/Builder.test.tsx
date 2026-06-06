import { fireEvent, render, screen, waitFor } from "@testing-library/react";
import { beforeEach, describe, expect, it, vi } from "vitest";
import { fetchUrdf, filterRobots, type RobotConfig } from "../api";
import { Builder } from "../Builder";

vi.mock("../api", async (orig) => {
  const actual = await orig<typeof import("../api")>();
  return {
    ...actual,
    filterRobots: vi.fn(),
    fetchUrdf: vi.fn(),
  };
});

// Viewer3D pulls in three.js/WebGL which jsdom cannot run; stub it.
vi.mock("../Viewer3D", () => ({
  Viewer3D: ({ urdfXml }: { urdfXml: string }) => (
    <div data-testid="viewer">{urdfXml.length} chars</div>
  ),
}));

const arms: RobotConfig[] = [
  { id: "ur3", display_name: "UR3", category: "universal_robots", type: "arm" },
  { id: "ur5", display_name: "UR5", category: "universal_robots", type: "arm" },
];
const ees: RobotConfig[] = [
  {
    id: "robotiq_2f85",
    display_name: "Robotiq 2F-85",
    category: "grippers",
    type: "end_effector",
  },
];
const bases: RobotConfig[] = [
  {
    id: "pedestal_large",
    display_name: "Pedestal (Large)",
    category: "bases",
    type: "base",
  },
];

beforeEach(() => {
  vi.mocked(filterRobots).mockImplementation(async (f) => {
    if (f?.type === "arm") return arms;
    if (f?.type === "end_effector") return ees;
    if (f?.type === "base") return bases;
    return [];
  });
  vi.mocked(fetchUrdf).mockResolvedValue({
    urdf_xml: "<robot/>",
    mesh_base: "/meshes/",
    missing_packages: [],
  });
});

describe("Builder", () => {
  it("shows arm cards on the active tab and an empty preview", async () => {
    render(<Builder />);
    expect(await screen.findByText("UR3")).toBeInTheDocument();
    expect(screen.getByText("UR5")).toBeInTheDocument();
    expect(
      screen.getByText(/select an arm to start building/i),
    ).toBeInTheDocument();
  });

  it("renders the live preview after picking an arm", async () => {
    render(<Builder />);
    fireEvent.click(await screen.findByText("UR3"));
    expect(await screen.findByTestId("viewer")).toBeInTheDocument();
    expect(fetchUrdf).toHaveBeenCalledWith("ur3");
  });

  it("disables fullscreen until an arm is picked, then opens a modal", async () => {
    render(<Builder />);
    await screen.findByText("UR3");
    const btn = screen.getByRole("button", { name: /fullscreen/i });
    expect(btn).toBeDisabled();

    fireEvent.click(screen.getByText("UR5"));
    await waitFor(() => expect(btn).not.toBeDisabled());

    fireEvent.click(btn);
    // Modal opens; the inline viewer is unmounted so only ONE WebGL viewer
    // is live at a time.
    expect(await screen.findByRole("dialog")).toBeInTheDocument();
    expect(screen.getAllByTestId("viewer")).toHaveLength(1);
    expect(screen.getByText(/viewing fullscreen/i)).toBeInTheDocument();
  });

  it("switches tabs to show end-effector cards (no None card)", async () => {
    render(<Builder />);
    await screen.findByText("UR3");
    fireEvent.click(screen.getByRole("tab", { name: /end-effector/i }));
    expect(await screen.findByText("Robotiq 2F-85")).toBeInTheDocument();
    // No standalone "None" card — clearing is done by toggling the card off.
    expect(
      screen.queryByRole("button", { name: "None" }),
    ).not.toBeInTheDocument();
    // Arm cards are hidden on this tab.
    expect(screen.queryByText("UR3")).not.toBeInTheDocument();
  });

  it("toggles a card off when clicked twice (select then unselect)", async () => {
    render(<Builder />);
    const ur5Card = await screen.findByRole("button", { name: "UR5 UR5" });
    // Select → tab pick reflects UR5, preview appears.
    fireEvent.click(ur5Card);
    expect(
      await screen.findByRole("tab", { name: /arm ur5/i }),
    ).toBeInTheDocument();
    expect(await screen.findByTestId("viewer")).toBeInTheDocument();

    // Click the same card again → cleared back to "Required", preview empty.
    fireEvent.click(screen.getByRole("button", { name: "UR5 UR5" }));
    expect(
      await screen.findByRole("tab", { name: /arm required/i }),
    ).toBeInTheDocument();
    expect(screen.queryByTestId("viewer")).not.toBeInTheDocument();
  });

  it("clears the whole assembly with the Clear button", async () => {
    render(<Builder />);
    const clear = await screen.findByRole("button", { name: /clear assembly/i });
    expect(clear).toBeDisabled();

    fireEvent.click(await screen.findByRole("button", { name: "UR5 UR5" }));
    await screen.findByTestId("viewer");
    await waitFor(() => expect(clear).not.toBeDisabled());

    fireEvent.click(clear);
    expect(
      await screen.findByRole("tab", { name: /arm required/i }),
    ).toBeInTheDocument();
    expect(screen.queryByTestId("viewer")).not.toBeInTheDocument();
    expect(clear).toBeDisabled();
  });
});
