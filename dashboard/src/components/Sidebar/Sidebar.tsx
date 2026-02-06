import { useState, useEffect } from 'react';

interface SidebarProps {
    currentView: string;
    onViewChange: (view: string) => void;
}

const Sidebar = ({ currentView, onViewChange }: SidebarProps) => {
    const [version, setVersion] = useState<number | null>(null);
    const [fallbackPort, setFallbackPort] = useState(false);

    useEffect(() => {
        fetch('http://localhost:3501/api/version')
            .then(res => res.json())
            .then(data => setVersion(data.version))
            .catch(console.error);

        fetch('http://localhost:3501/api/bridge-status')
            .then(res => res.json())
            .then(data => setFallbackPort(data.fallbackPort))
            .catch(console.error);
    }, []);

    const menuItems = [
        { id: 'apps', label: 'Apps', icon: '🚀' },
        { id: 'logs', label: 'Monitor', icon: '💎' },
        { id: 'configs', label: 'Macros', icon: '⚡' },
    ];

    return (
        <aside className="sidebar glass">
            <div className="sidebar-header">
                <h2>Automate.ly</h2>
            </div>

            <nav className="sidebar-nav" style={{ flex: 1 }}>
                {menuItems.map((item) => (
                    <button
                        key={item.id}
                        className={`nav-item ${currentView === item.id ? 'active' : ''}`}
                        onClick={() => onViewChange(item.id)}
                    >
                        <span className="icon" style={{ fontSize: '1.2rem' }}>{item.icon}</span>
                        <span className="label">{item.label}</span>
                    </button>
                ))}
            </nav>

            <div className="sidebar-footer" style={{ marginTop: 'auto', paddingTop: '20px', borderTop: '1px solid var(--glass-border)' }}>
                {fallbackPort && (
                    <div style={{
                        fontSize: '0.75rem',
                        color: '#ff9800',
                        background: 'rgba(255, 152, 0, 0.1)',
                        border: '1px solid rgba(255, 152, 0, 0.3)',
                        borderRadius: '6px',
                        padding: '6px 10px',
                        marginBottom: '10px',
                    }}>
                        Bridge using fallback port (no leader)
                    </div>
                )}
                <div className="status-indicator">
                    <span className="dot pulse" style={{
                        width: '8px',
                        height: '8px',
                        background: 'var(--success)',
                        borderRadius: '50%',
                        marginRight: '10px',
                        display: 'inline-block'
                    }}></span>
                    <span style={{ fontSize: '0.85rem', fontWeight: '600', color: 'var(--text-secondary)' }}>
                        Daemon: <span style={{ color: 'var(--success)' }}>Active</span>
                        {version !== null && <span style={{ marginLeft: '8px', opacity: 0.7 }}>v{version}</span>}
                    </span>
                </div>
            </div>
        </aside>
    );
};

export default Sidebar;
