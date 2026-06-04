import { fireEvent, render, screen } from "@testing-library/react";
import { describe, expect, it, vi } from "vitest";
import { StartScreen } from "../StartScreen";

describe("StartScreen", () => {
  it("shows a title and a begin button", () => {
    render(<StartScreen onBegin={vi.fn()} />);
    expect(screen.getByRole("heading", { level: 1 })).toBeInTheDocument();
    expect(screen.getByRole("button", { name: /browse robots/i })).toBeInTheDocument();
  });

  it("calls onBegin when the button is clicked", () => {
    const onBegin = vi.fn();
    render(<StartScreen onBegin={onBegin} />);
    fireEvent.click(screen.getByRole("button", { name: /browse robots/i }));
    expect(onBegin).toHaveBeenCalled();
  });
});
