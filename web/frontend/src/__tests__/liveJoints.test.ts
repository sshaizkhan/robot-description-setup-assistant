import { describe, expect, it } from "vitest";
import { parseJointMessage } from "../liveJoints";

describe("parseJointMessage", () => {
  it("extracts the joints map", () => {
    expect(parseJointMessage('{"joints":{"a":0.5,"b":1}}')).toEqual({
      a: 0.5,
      b: 1,
    });
  });

  it("returns an empty map when joints are absent", () => {
    expect(parseJointMessage('{"error":"no ros"}')).toEqual({});
  });
});
