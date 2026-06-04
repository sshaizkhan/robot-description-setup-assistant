import { render, screen } from "@testing-library/react";
import { describe, expect, it } from "vitest";
import type { RobotConfig, ValidationResult } from "../api";
import { DetailPanel } from "../DetailPanel";

const ur3: RobotConfig = {
  id: "ur3",
  display_name: "UR3",
  category: "universal_robots",
  description: "Compact arm",
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
};

describe("DetailPanel", () => {
  it("shows a placeholder when no robot is selected", () => {
    render(<DetailPanel robot={null} validation={null} />);
    expect(screen.getByText(/Select a robot/i)).toBeInTheDocument();
  });

  it("renders specs for the selected robot", () => {
    render(<DetailPanel robot={ur3} validation={null} />);
    expect(screen.getByText("UR3")).toBeInTheDocument();
    expect(screen.getByText("Compact arm")).toBeInTheDocument();
    expect(screen.getByText(/6/)).toBeInTheDocument();
  });

  it("offers a download link for the bringup package", () => {
    render(<DetailPanel robot={ur3} validation={null} />);
    const link = screen.getByRole("link", { name: /download .* package/i });
    expect(link).toHaveAttribute("href", "/api/robots/ur3/package");
  });

  it("warns about missing packages", () => {
    const validation: ValidationResult = {
      ok: false,
      missing_packages: ["ur_msgs"],
    };
    render(<DetailPanel robot={ur3} validation={validation} />);
    expect(screen.getByText(/Missing packages/i)).toBeInTheDocument();
    expect(screen.getByText("ur_msgs")).toBeInTheDocument();
  });

  it("does not crash when specifications are missing (null-deref guard)", () => {
    const bare: RobotConfig = { id: "x", display_name: "X", category: "c" };
    render(<DetailPanel robot={bare} validation={null} />);
    expect(screen.getByText("X")).toBeInTheDocument();
  });
});
