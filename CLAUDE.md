# Project notes

## Build constraints

- **Old/low-RAM machine.** NEVER run `colcon build` across all packages with default parallelism.
  Always pass `--parallel-workers 1` (or at most `2`). Prefer building one package at a time with
  `--packages-select <pkg>`. Example:

  ```bash
  colcon build --parallel-workers 1 --packages-select robot_catalog_core
  ```
