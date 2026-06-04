import { render, screen, waitFor } from "@testing-library/react";
import { describe, expect, it, vi } from "vitest";
import { Intro } from "../Intro";

describe("Intro", () => {
  it("types out lines then calls onDone", async () => {
    const onDone = vi.fn();
    render(<Intro onDone={onDone} />);

    await waitFor(() =>
      expect(screen.getByText("$ rdsa --init")).toBeInTheDocument(),
    );
    await waitFor(() => expect(onDone).toHaveBeenCalled(), { timeout: 6000 });
  });
});
