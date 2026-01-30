import type { Peer } from '../peers';
import { PEERS, STORAGE_KEY } from '../peers';

interface Props {
  peer: Peer;
  onChange: (peer: Peer) => void;
}

const PeerSelector = ({ peer, onChange }: Props) => {
  const handleChange = (e: React.ChangeEvent<HTMLSelectElement>) => {
    const selected = PEERS.find(p => p.id === e.target.value);
    if (selected) {
      localStorage.setItem(STORAGE_KEY, selected.id);
      onChange(selected);
    }
  };

  return (
    <div style={{ display: 'flex', alignItems: 'center', gap: '8px' }}>
      <span style={{
        fontSize: '0.7rem',
        color: 'var(--text-dim)',
        textTransform: 'uppercase',
        letterSpacing: '1.5px',
        fontWeight: '700'
      }}>Peer</span>
      <select
        value={peer.id}
        onChange={handleChange}
        style={{
          background: 'rgba(255, 255, 255, 0.05)',
          border: '1px solid rgba(255, 255, 255, 0.1)',
          borderRadius: '6px',
          color: 'var(--accent-cyan)',
          padding: '4px 8px',
          fontSize: '0.9rem',
          fontWeight: '600',
          cursor: 'pointer',
          outline: 'none'
        }}
      >
        {PEERS.map(p => (
          <option key={p.id} value={p.id} style={{ background: '#1a1a2e', color: '#fff' }}>
            {p.name} ({p.ip})
          </option>
        ))}
      </select>
    </div>
  );
};

export default PeerSelector;
