import type { RobotConfig } from "./api";
import { RobotCard } from "./RobotCard";

interface RobotGridProps {
  robots: RobotConfig[];
  selectedId: string | null;
  onSelect: (id: string) => void;
  total: number;
  onLoadMore: () => void;
}

export function RobotGrid({
  robots,
  selectedId,
  onSelect,
  total,
  onLoadMore,
}: RobotGridProps) {
  const hasMore = robots.length < total;
  return (
    <section className="robot-grid-section">
      <p className="robot-count">
        {hasMore ? `Showing ${robots.length} of ${total}` : `${total} robots`}
      </p>
      {robots.length === 0 ? (
        <p className="empty-state">No robots match the current filters.</p>
      ) : (
        <>
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
          {hasMore && (
            <button type="button" className="load-more" onClick={onLoadMore}>
              Load more
            </button>
          )}
        </>
      )}
    </section>
  );
}
