import { describe, expect, it, vi } from "vitest";
import { connectTf, type TfTransform } from "../tfSocket";
import { connectMarkers, type RvizMarker } from "../markerSocket";

class FakeWS {
  onmessage: ((e: MessageEvent) => void) | null = null;
  close = vi.fn();
  constructor(public url: string) {}
  emit(data: unknown) {
    this.onmessage?.({ data: JSON.stringify(data) } as MessageEvent);
  }
}

describe("connectTf", () => {
  it("parses transforms and closes", () => {
    let ws!: FakeWS;
    const WS = vi.fn((url: string) => (ws = new FakeWS(url))) as never;
    let got: TfTransform[] = [];
    const close = connectTf("ws://x/ws/tf", (t) => (got = t), WS);
    ws.emit({ transforms: [{ parent: "world", child: "base" }] });
    expect(got[0].parent).toBe("world");
    close();
    expect(ws.close).toHaveBeenCalled();
  });
});

describe("connectMarkers", () => {
  it("parses markers and closes", () => {
    let ws!: FakeWS;
    const WS = vi.fn((url: string) => (ws = new FakeWS(url))) as never;
    let got: RvizMarker[] = [];
    const close = connectMarkers("ws://x/ws/markers", (m) => (got = m), WS);
    ws.emit({ markers: [{ ns: "a", id: 1, type: 2 }] });
    expect(got[0].id).toBe(1);
    close();
    expect(ws.close).toHaveBeenCalled();
  });
});
