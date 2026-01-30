export interface Peer {
  id: string;
  name: string;
  ip: string;
}

export const PEERS: Peer[] = [
  { id: 'desktop', name: 'Desktop', ip: '10.0.0.2' },
  { id: 'vps', name: 'VPS', ip: '10.0.0.1' },
  { id: 'laptop', name: 'Laptop', ip: '10.0.0.4' },
];

export const STORAGE_KEY = 'dashboard-selected-peer';
