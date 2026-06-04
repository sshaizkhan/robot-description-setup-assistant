import { fireEvent, render, screen } from "@testing-library/react";
import { describe, expect, it, vi } from "vitest";
import { Select } from "../Select";

const options = [
  { value: "", label: "All" },
  { value: "kuka", label: "KUKA Robots" },
];

describe("Select", () => {
  it("shows the current value and opens on click", () => {
    render(<Select label="Category" value="" options={options} onChange={vi.fn()} />);
    const trigger = screen.getByRole("button", { name: /category/i });
    expect(trigger).toHaveAttribute("aria-expanded", "false");
    fireEvent.click(trigger);
    expect(trigger).toHaveAttribute("aria-expanded", "true");
    expect(screen.getByRole("listbox")).toBeInTheDocument();
  });

  it("emits the chosen value and closes", () => {
    const onChange = vi.fn();
    render(<Select label="Category" value="" options={options} onChange={onChange} />);
    fireEvent.click(screen.getByRole("button", { name: /category/i }));
    fireEvent.click(screen.getByText("KUKA Robots"));
    expect(onChange).toHaveBeenCalledWith("kuka");
    expect(screen.queryByRole("listbox")).not.toBeInTheDocument();
  });

  it("closes on outside click", () => {
    render(<Select label="Category" value="" options={options} onChange={vi.fn()} />);
    fireEvent.click(screen.getByRole("button", { name: /category/i }));
    expect(screen.getByRole("listbox")).toBeInTheDocument();
    fireEvent.mouseDown(document.body);
    expect(screen.queryByRole("listbox")).not.toBeInTheDocument();
  });
});
