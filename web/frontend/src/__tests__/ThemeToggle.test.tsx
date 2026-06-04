import { fireEvent, render, screen } from "@testing-library/react";
import { describe, expect, it, vi } from "vitest";
import { ThemeToggle } from "../ThemeToggle";

describe("ThemeToggle", () => {
  it("reflects the current theme via aria-pressed", () => {
    const { rerender } = render(<ThemeToggle theme="light" onToggle={vi.fn()} />);
    const btn = screen.getByRole("button", { name: /toggle dark mode/i });
    expect(btn).toHaveAttribute("aria-pressed", "false");
    rerender(<ThemeToggle theme="dark" onToggle={vi.fn()} />);
    expect(btn).toHaveAttribute("aria-pressed", "true");
  });

  it("fires onToggle when clicked", () => {
    const onToggle = vi.fn();
    render(<ThemeToggle theme="light" onToggle={onToggle} />);
    fireEvent.click(screen.getByRole("button", { name: /toggle dark mode/i }));
    expect(onToggle).toHaveBeenCalled();
  });
});
