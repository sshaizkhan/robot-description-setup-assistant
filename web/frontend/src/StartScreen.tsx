import { ThemeToggle, type Theme } from "./ThemeToggle";

interface StartScreenProps {
  onBegin: () => void;
  theme?: Theme;
  onThemeToggle?: () => void;
}

export function StartScreen({ onBegin, theme, onThemeToggle }: StartScreenProps) {
  return (
    <div className="start-screen">
      {onThemeToggle && (
        <div className="start-actions">
          <ThemeToggle theme={theme ?? "light"} onToggle={onThemeToggle} />
        </div>
      )}
      <h1>Robot Description Setup Assistant</h1>
      <p>Browse the robot catalog and preview robots in 3D.</p>
      <button type="button" onClick={onBegin}>
        Browse robots
      </button>
    </div>
  );
}
