import React from 'react';

interface SidebarProps {
    currentView: string;
    onViewChange: (view: string) => void;
}

const Sidebar: React.FC<SidebarProps> = ({ currentView, onViewChange }) => {
    const menuItems = [
        { id: 'logs', label: 'Live Logs', icon: '📝' },
        { id: 'configs', label: 'Configurations', icon: '⚙️' },
    ];

    return (
        <div className="sidebar glass">
            <div className="sidebar-header">
                <h2>AutomateLinux</h2>
            </div>
            <nav className="sidebar-nav">
                {menuItems.map((item) => (
                    <button
                        key={item.id}
                        className={`nav-item ${currentView === item.id ? 'active' : ''}`}
                        onClick={() => onViewChange(item.id)}
                    >
                        <span className="icon">{item.icon}</span>
                        <span className="label">{item.label}</span>
                    </button>
                ))}
            </nav>
            <div className="sidebar-footer">
                <div className="status-indicator">
                    <span className="dot pulse"></span> Daemon Active
                </div>
            </div>
        </div>
    );
};

export default Sidebar;
