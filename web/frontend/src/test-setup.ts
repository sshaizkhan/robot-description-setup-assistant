import "@testing-library/jest-dom/vitest";

// jsdom has no ResizeObserver; the virtualized grid (and @tanstack/react-virtual)
// rely on it. Provide a no-op stub so components mount in tests.
class ResizeObserverStub {
  observe(): void {}
  unobserve(): void {}
  disconnect(): void {}
}
if (!("ResizeObserver" in globalThis)) {
  (globalThis as unknown as { ResizeObserver: unknown }).ResizeObserver =
    ResizeObserverStub;
}
