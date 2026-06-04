import { parseJointMessage } from "./liveJoints";

/**
 * Connect to the live joint-state WebSocket. Calls `onJoints` with each parsed
 * joint map. Returns a function that closes the socket.
 */
export function connectJointStates(
  url: string,
  onJoints: (joints: Record<string, number>) => void,
  WS: typeof WebSocket = WebSocket,
): () => void {
  const ws = new WS(url);
  ws.onmessage = (e: MessageEvent) => onJoints(parseJointMessage(e.data));
  return () => ws.close();
}
