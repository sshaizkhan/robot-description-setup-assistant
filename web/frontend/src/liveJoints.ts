export interface JointStateMessage {
  joints?: Record<string, number>;
  error?: string;
}

export function parseJointMessage(data: string): Record<string, number> {
  const msg = JSON.parse(data) as JointStateMessage;
  return msg.joints ?? {};
}
