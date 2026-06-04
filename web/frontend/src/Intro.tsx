import { useEffect, useState } from "react";

const LINES = [
  "$ rdsa --init",
  "booting robot description setup assistant",
  "",
  "for (const robot of catalog) {",
  "  load(robot.urdf)       ok",
  "  resolve(meshes)        ok",
  "  validate(packages)     ok",
  "}",
  "",
  "→ 20 robots ready",
];

const STEP_MS = 130;

interface IntroProps {
  onDone: () => void;
}

export function Intro({ onDone }: IntroProps) {
  const [shown, setShown] = useState(0);
  const [leaving, setLeaving] = useState(false);

  useEffect(() => {
    const reduce =
      typeof window !== "undefined" &&
      window.matchMedia?.("(prefers-reduced-motion: reduce)").matches;
    if (reduce) {
      onDone();
      return;
    }
    if (shown < LINES.length) {
      const t = setTimeout(() => setShown((n) => n + 1), STEP_MS);
      return () => clearTimeout(t);
    }
    const t1 = setTimeout(() => setLeaving(true), 520);
    const t2 = setTimeout(onDone, 940);
    return () => {
      clearTimeout(t1);
      clearTimeout(t2);
    };
  }, [shown, onDone]);

  return (
    <div className={`intro${leaving ? " intro-leaving" : ""}`} role="presentation">
      <pre className="intro-code">
        {LINES.slice(0, shown).map((line, i) => (
          <span className="intro-line" key={i}>
            {line || " "}
          </span>
        ))}
        {shown < LINES.length && <span className="intro-cursor">▋</span>}
      </pre>
    </div>
  );
}
