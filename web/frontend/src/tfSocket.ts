export interface Vec3 {
  x: number;
  y: number;
  z: number;
}
export interface Quat {
  x: number;
  y: number;
  z: number;
  w: number;
}
export interface TfTransform {
  parent: string;
  child: string;
  translation: Vec3;
  rotation: Quat;
}

/** Connect to /ws/tf. Calls `onTf` with each transform array. Returns closer. */
export function connectTf(
  url: string,
  onTf: (transforms: TfTransform[]) => void,
  WS: typeof WebSocket = WebSocket,
): () => void {
  const ws = new WS(url);
  ws.onmessage = (e: MessageEvent) => {
    const msg = JSON.parse(e.data as string);
    onTf((msg.transforms ?? []) as TfTransform[]);
  };
  return () => ws.close();
}
