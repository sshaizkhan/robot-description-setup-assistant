import { fireEvent, render, screen } from "@testing-library/react";
import { describe, expect, it, vi } from "vitest";
import { Modal } from "../Modal";

describe("Modal", () => {
  it("renders a title and children", () => {
    render(<Modal onClose={vi.fn()} title="UR3"><p>body</p></Modal>);
    expect(screen.getByText("UR3")).toBeInTheDocument();
    expect(screen.getByText("body")).toBeInTheDocument();
  });

  it("closes via the close button", () => {
    const onClose = vi.fn();
    render(<Modal onClose={onClose}><p>body</p></Modal>);
    fireEvent.click(screen.getByRole("button", { name: /close viewer/i }));
    expect(onClose).toHaveBeenCalled();
  });

  it("closes when the backdrop is clicked but not the dialog", () => {
    const onClose = vi.fn();
    render(<Modal onClose={onClose}><p>inside</p></Modal>);
    fireEvent.click(screen.getByText("inside"));
    expect(onClose).not.toHaveBeenCalled();
    fireEvent.click(screen.getByRole("dialog").parentElement!);
    expect(onClose).toHaveBeenCalled();
  });

  it("closes on Escape", () => {
    const onClose = vi.fn();
    render(<Modal onClose={onClose}><p>body</p></Modal>);
    fireEvent.keyDown(window, { key: "Escape" });
    expect(onClose).toHaveBeenCalled();
  });
});
