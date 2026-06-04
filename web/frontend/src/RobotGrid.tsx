import type { RobotConfig } from "./api";
import { RobotCard } from "./RobotCard";

interface RobotGridProps {
  robots: RobotConfig[];
  selectedId: string | null;
  onSelect: (id: string) => void;
}

export function RobotGrid({ robots, selectedId, onSelect }: RobotGridProps) {
  return (
    <section className="robot-grid-section">
      <p className="robot-count">{robots.length} robots</p>
      {robots.length === 0 ? (
        <p className="empty-state">No robots match the current filters.</p>
      ) : (
        <div className="robot-grid">
          {robots.map((r) => (
            <RobotCard
              key={r.id}
              robot={r}
              selected={r.id === selectedId}
              onSelect={onSelect}
            />
          ))}
        </div>
      )}
    </section>
  );
}
