import { render, screen } from "@testing-library/react";
import { describe, expect, it } from "vitest";
import { RvizPanel } from "../RvizPanel";

describe("RvizPanel", () => {
  it("renders the panel scaffolding and a status line", () => {
    render(<RvizPanel urdfXml={null} meshBase="/meshes" />);
    expect(screen.getByTestId("rviz-panel")).toBeInTheDocument();
    expect(screen.getByTestId("rviz-status")).toBeInTheDocument();
  });

  it("shows the empty state when no URDF is loaded", () => {
    render(<RvizPanel urdfXml={null} meshBase="/meshes" />);
    expect(screen.getByText(/no robot/i)).toBeInTheDocument();
  });
});
