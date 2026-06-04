import { fireEvent, render, screen } from "@testing-library/react";
import { describe, expect, it, vi } from "vitest";
import type { RobotConfig } from "../api";
import { RobotGrid } from "../RobotGrid";

const robots: RobotConfig[] = [
  {
    id: "ur3",
    display_name: "UR3",
    category: "universal_robots",
    specifications: {
      degrees_of_freedom: 6,
      payload_kg: 3,
      reach_mm: 500,
      weight_kg: 11,
      repeatability_mm: 0.1,
      max_speed_ms: 1,
      mounting_options: [],
      safety_certified: true,
      collaborative: true,
      torque_sensing: false,
    },
  },
  { id: "ur5", display_name: "UR5", category: "universal_robots" },
];

describe("RobotGrid", () => {
  it("renders a card per robot and the count", () => {
    render(<RobotGrid robots={robots} selectedId={null} onSelect={vi.fn()} />);
    expect(screen.getByText("UR3")).toBeInTheDocument();
    expect(screen.getByText("UR5")).toBeInTheDocument();
    expect(screen.getByText("2 robots")).toBeInTheDocument();
  });

  it("calls onSelect with the robot id when a card is clicked", () => {
    const onSelect = vi.fn();
    render(<RobotGrid robots={robots} selectedId={null} onSelect={onSelect} />);
    fireEvent.click(screen.getByText("UR3"));
    expect(onSelect).toHaveBeenCalledWith("ur3");
  });

  it("renders an empty state when there are no robots", () => {
    render(<RobotGrid robots={[]} selectedId={null} onSelect={vi.fn()} />);
    expect(screen.getByText("0 robots")).toBeInTheDocument();
    expect(screen.getByText(/No robots match/i)).toBeInTheDocument();
  });
});
