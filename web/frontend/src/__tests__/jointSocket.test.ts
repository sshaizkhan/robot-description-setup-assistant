import { describe, expect, it, vi } from "vitest";
import { connectJointStates } from "../jointSocket";

class MockSocket {
  url: string;
  onmessage: ((e: { data: string }) => void) | null = null;
  closed = false;
  constructor(url: string) {
    this.url = url;
  }
  close() {
    this.closed = true;
  }
}

describe("connectJointStates", () => {
  it("parses messages and forwards joints", () => {
    const onJoints = vi.fn();
    let made: TrackedSocket | null = null;

    class TrackedSocket extends MockSocket {
      constructor(url: string) {
        super(url);
        made = this;
      }
    }

    const close = connectJointStates(
      "ws://x/ws/joint_states",
      onJoints,
      TrackedSocket as unknown as typeof WebSocket,
    );
    made!.onmessage?.({ data: '{"joints":{"a":0.5}}' });
    expect(onJoints).toHaveBeenCalledWith({ a: 0.5 });

    close();
    expect(made!.closed).toBe(true);
  });
});
