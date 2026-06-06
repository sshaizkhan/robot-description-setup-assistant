import { useEffect, useLayoutEffect, useRef, useState } from "react";
import { useWindowVirtualizer } from "@tanstack/react-virtual";
import type { RobotConfig } from "./api";
import { RobotCard } from "./RobotCard";

interface RobotGridProps {
  robots: RobotConfig[];
  selectedId: string | null;
  onSelect: (id: string) => void;
  total: number;
  onLoadMore: () => void;
}

const GAP = 20; // must match the row gap below
const EST_ROW = 360; // initial row-height guess; real height is measured

// Columns by container width (mirrors the old CSS breakpoints). Falls back to 1
// when width is unknown (e.g. jsdom) so all rows still render in tests.
function useColumns(ref: React.RefObject<HTMLDivElement>): number {
  const [cols, setCols] = useState(3);
  useEffect(() => {
    const el = ref.current;
    if (!el) return;
    const compute = () => {
      // Column count from the grid's own width (cards are ~270px min).
      const w = el.clientWidth || 0;
      setCols(w === 0 ? 1 : w < 540 ? 1 : w < 820 ? 2 : 3);
    };
    compute();
    const ro = new ResizeObserver(compute);
    ro.observe(el);
    return () => ro.disconnect();
  }, [ref]);
  return cols;
}

export function RobotGrid({
  robots,
  selectedId,
  onSelect,
  total,
  onLoadMore,
}: RobotGridProps) {
  const parentRef = useRef<HTMLDivElement>(null);
  const cols = useColumns(parentRef);
  const hasMore = robots.length < total;
  const rowCount = Math.ceil(robots.length / cols);

  // Window-scroll virtualizer: only rows near the viewport are in the DOM.
  // scrollMargin = the grid's distance from the top of the document.
  const [scrollMargin, setScrollMargin] = useState(0);
  useLayoutEffect(() => {
    const el = parentRef.current;
    if (!el) return;
    const measure = () =>
      setScrollMargin(el.getBoundingClientRect().top + window.scrollY);
    measure();
    window.addEventListener("resize", measure);
    return () => window.removeEventListener("resize", measure);
  }, [robots.length]);

  const virtualizer = useWindowVirtualizer({
    count: rowCount,
    estimateSize: () => EST_ROW + GAP,
    overscan: 4,
    scrollMargin,
  });

  return (
    <section className="robot-grid-section">
      <p className="robot-count">
        {hasMore ? `Showing ${robots.length} of ${total}` : `${total} robots`}
      </p>
      {robots.length === 0 ? (
        <p className="empty-state">No robots match the current filters.</p>
      ) : (
        <>
          <div ref={parentRef} className="robot-grid-virt">
            <div
              style={{
                height: virtualizer.getTotalSize(),
                position: "relative",
                width: "100%",
              }}
            >
              {virtualizer.getVirtualItems().map((vRow) => {
                const start = vRow.index * cols;
                const rowItems = robots.slice(start, start + cols);
                return (
                  <div
                    key={vRow.key}
                    data-index={vRow.index}
                    ref={virtualizer.measureElement}
                    className="robot-row"
                    style={{
                      position: "absolute",
                      top: 0,
                      left: 0,
                      width: "100%",
                      transform: `translateY(${vRow.start - scrollMargin}px)`,
                      gridTemplateColumns: `repeat(${cols}, minmax(0, 1fr))`,
                      gap: `${GAP}px`,
                      paddingBottom: `${GAP}px`,
                    }}
                  >
                    {rowItems.map((r) => (
                      <RobotCard
                        key={r.id}
                        robot={r}
                        selected={r.id === selectedId}
                        onSelect={onSelect}
                      />
                    ))}
                  </div>
                );
              })}
            </div>
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
