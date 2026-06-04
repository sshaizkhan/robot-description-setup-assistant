import { fireEvent, render, screen } from "@testing-library/react";
import { describe, expect, it, vi } from "vitest";
import { FilterPanel } from "../FilterPanel";

const categories = [
  { id: "universal_robots", display_name: "Universal Robots" },
  { id: "kuka", display_name: "KUKA Robots" },
];

describe("FilterPanel", () => {
  it("emits a filter when the search text changes", () => {
    const onChange = vi.fn();
    render(<FilterPanel categories={categories} onChange={onChange} />);
    fireEvent.change(screen.getByPlaceholderText("Search robots..."), {
      target: { value: "ur" },
    });
    expect(onChange).toHaveBeenLastCalledWith({ search_text: "ur" });
  });

  it("emits a category filter when a category is picked", () => {
    const onChange = vi.fn();
    render(<FilterPanel categories={categories} onChange={onChange} />);
    fireEvent.click(screen.getByRole("button", { name: /category/i }));
    fireEvent.click(screen.getByText("KUKA Robots"));
    expect(onChange).toHaveBeenLastCalledWith({ category: "kuka" });
  });

  it("clears all filters", () => {
    const onChange = vi.fn();
    render(<FilterPanel categories={categories} onChange={onChange} />);
    fireEvent.change(screen.getByPlaceholderText("Search robots..."), {
      target: { value: "ur" },
    });
    fireEvent.click(screen.getByText("Clear"));
    expect(onChange).toHaveBeenLastCalledWith({});
  });
});
