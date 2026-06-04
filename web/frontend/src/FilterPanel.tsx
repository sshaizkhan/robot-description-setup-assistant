import { useState } from "react";
import type { CategoryInfo, RobotFilter } from "./api";
import { buildRobotFilter, EMPTY_FORM, type FilterForm } from "./robotFilter";
import { Select } from "./Select";

interface FilterPanelProps {
  categories: CategoryInfo[];
  onChange: (filter: RobotFilter) => void;
}

export function FilterPanel({ categories, onChange }: FilterPanelProps) {
  const [form, setForm] = useState<FilterForm>(EMPTY_FORM);

  const update = (patch: Partial<FilterForm>) => {
    const next = { ...form, ...patch };
    setForm(next);
    onChange(buildRobotFilter(next));
  };

  const clear = () => {
    setForm(EMPTY_FORM);
    onChange({});
  };

  return (
    <aside className="filter-panel">
      <h2>Filters</h2>
      <input
        type="text"
        placeholder="Search robots..."
        value={form.searchText}
        onChange={(e) => update({ searchText: e.target.value })}
      />
      <Select
        label="Category"
        value={form.category}
        options={[
          { value: "", label: "All" },
          ...categories.map((c) => ({ value: c.id, label: c.display_name })),
        ]}
        onChange={(value) => update({ category: value })}
      />
      <label>
        Min payload (kg)
        <input
          type="number"
          value={form.minPayload}
          onChange={(e) => update({ minPayload: e.target.value })}
        />
      </label>
      <label>
        Max payload (kg)
        <input
          type="number"
          value={form.maxPayload}
          onChange={(e) => update({ maxPayload: e.target.value })}
        />
      </label>
      <label>
        Min reach (mm)
        <input
          type="number"
          value={form.minReach}
          onChange={(e) => update({ minReach: e.target.value })}
        />
      </label>
      <label>
        Max reach (mm)
        <input
          type="number"
          value={form.maxReach}
          onChange={(e) => update({ maxReach: e.target.value })}
        />
      </label>
      <label>
        DOF
        <input
          type="number"
          value={form.dof}
          onChange={(e) => update({ dof: e.target.value })}
        />
      </label>
      <label>
        <input
          type="checkbox"
          checked={form.collaborativeOnly}
          onChange={(e) => update({ collaborativeOnly: e.target.checked })}
        />
        Collaborative only
      </label>
      <button onClick={clear}>Clear</button>
    </aside>
  );
}
