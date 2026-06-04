import { packageUrl, type RobotConfig, type ValidationResult } from "./api";

interface DetailPanelProps {
  robot: RobotConfig | null;
  validation: ValidationResult | null;
}

export function DetailPanel({ robot, validation }: DetailPanelProps) {
  if (!robot) {
    return (
      <aside className="detail-panel">
        <p className="placeholder">Select a robot to see details.</p>
      </aside>
    );
  }

  const spec = robot.specifications;
  return (
    <aside className="detail-panel">
      <h2>{robot.display_name}</h2>
      {robot.description && <p>{robot.description}</p>}
      <p>
        <a className="download-pkg" href={packageUrl(robot.id)}>
          Download bringup package
        </a>
      </p>
      {spec && (
        <table className="spec-table">
          <tbody>
            <tr><td>DOF</td><td>{spec.degrees_of_freedom}</td></tr>
            <tr><td>Payload</td><td>{spec.payload_kg} kg</td></tr>
            <tr><td>Reach</td><td>{spec.reach_mm} mm</td></tr>
            <tr><td>Weight</td><td>{spec.weight_kg} kg</td></tr>
            <tr><td>Repeatability</td><td>{spec.repeatability_mm} mm</td></tr>
            <tr><td>Collaborative</td><td>{spec.collaborative ? "Yes" : "No"}</td></tr>
          </tbody>
        </table>
      )}
      {validation && validation.missing_packages.length > 0 && (
        <div className="missing-packages" role="alert">
          <strong>Missing packages:</strong>
          <ul>
            {validation.missing_packages.map((p) => (
              <li key={p}>{p}</li>
            ))}
          </ul>
        </div>
      )}
    </aside>
  );
}
