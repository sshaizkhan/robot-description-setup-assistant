import lockupLight from "../resources/RDSA-lockup-light-nobg.png";
import lockupDark from "../resources/RDSA-lockup-dark-nobg.png";
import type { Theme } from "./ThemeToggle";

interface LogoProps {
  theme: Theme;
  className?: string;
}

/**
 * Brand lockup that swaps with the active theme: the "-light" art is drawn for
 * light backgrounds, the "-dark" art for dark backgrounds.
 */
export function Logo({ theme, className }: LogoProps) {
  return (
    <img
      className={className ? `app-logo ${className}` : "app-logo"}
      src={theme === "dark" ? lockupDark : lockupLight}
      alt="Robot Description Setup Assistant"
    />
  );
}
