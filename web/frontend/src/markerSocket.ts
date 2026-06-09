import type { Vec3, Quat } from "./tfSocket";

export interface RvizMarker {
  ns: string;
  id: number;
  type: number;
  action: number;
  frame_id: string;
  pose: { position: Vec3; orientation: Quat };
  scale: Vec3;
  color: { r: number; g: number; b: number; a: number };
  points: Vec3[];
  text: string;
  lifetime: number;
}

/** Connect to /ws/markers. Calls `onMarkers` with each array. Returns closer. */
export function connectMarkers(
  url: string,
  onMarkers: (markers: RvizMarker[]) => void,
  WS: typeof WebSocket = WebSocket,
): () => void {
  const ws = new WS(url);
  ws.onmessage = (e: MessageEvent) => {
    const msg = JSON.parse(e.data as string);
    onMarkers((msg.markers ?? []) as RvizMarker[]);
  };
  return () => ws.close();
}
