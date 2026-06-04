import { describe, expect, it } from "vitest";
import { buildRobotFilter, EMPTY_FORM, type FilterForm } from "../robotFilter";

describe("buildRobotFilter", () => {
  it("omits empty fields", () => {
    expect(buildRobotFilter(EMPTY_FORM)).toEqual({});
  });

  it("maps populated fields to a RobotFilter", () => {
    const form: FilterForm = {
      searchText: "ur",
      category: "universal_robots",
      minPayload: "5",
      maxPayload: "",
      minReach: "",
      maxReach: "",
      dof: "6",
      collaborativeOnly: true,
    };
    expect(buildRobotFilter(form)).toEqual({
      search_text: "ur",
      category: "universal_robots",
      min_payload: 5,
      degrees_of_freedom: 6,
      collaborative_only: true,
    });
  });

  it("ignores non-numeric numeric fields", () => {
    const form: FilterForm = { ...EMPTY_FORM, minPayload: "abc" };
    expect(buildRobotFilter(form)).toEqual({});
  });
});
