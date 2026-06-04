import type { RobotConfig } from "./api";
import { imageUrl } from "./api";

interface RobotCardProps {
  robot: RobotConfig;
  selected: boolean;
  onSelect: (id: string) => void;
}

export function RobotCard({ robot, selected, onSelect }: RobotCardProps) {
  const spec = robot.specifications;
  return (
    <button
      type="button"
      className={`robot-card${selected ? " selected" : ""}`}
      onClick={() => onSelect(robot.id)}
    >
      <img
        className="robot-card-img"
        src={imageUrl(robot.id)}
        alt={robot.display_name}
        onError={(e) => {
          (e.currentTarget as HTMLImageElement).style.visibility = "hidden";
        }}
      />
      <div className="robot-card-name">{robot.display_name}</div>
      {spec && (
        <div className="robot-card-specs">
          {spec.degrees_of_freedom} DOF · {spec.payload_kg} kg · {spec.reach_mm} mm
        </div>
      )}
    </button>
  );
}
