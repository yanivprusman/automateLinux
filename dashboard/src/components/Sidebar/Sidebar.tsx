

interface SidebarProps {
    currentView: string;
    onViewChange: (view: string) => void;
}

const Sidebar = ({ currentView, onViewChange }: SidebarProps) => {
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
                        Daemon System: <span style={{ color: 'var(--success)' }}>Active</span>
                    </span>
                </div>
            </div>
        </aside>
    );
};

export default Sidebar;
