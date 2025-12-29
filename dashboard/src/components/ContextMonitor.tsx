import React, { useEffect, useState } from 'react';

interface ContextData {
    activeApp: string;
    activeUrl: string;
    activeTitle: string;
    numLockActive: boolean;
}

const ContextMonitor: React.FC = () => {
    const [context, setContext] = useState<ContextData>({
        activeApp: 'loading...',
        activeUrl: '',
        activeTitle: '',
        numLockActive: false
    });

    useEffect(() => {
        const fetchContext = () => {
            fetch('http://localhost:9224/api/activeContext')
                .then(res => res.json())
                .then(data => setContext(data))
                .catch(err => console.error("Context fetch failed", err));
        };

        fetchContext(); // Initial fetch
        const interval = setInterval(fetchContext, 1000); // Poll every second

        return () => clearInterval(interval);
    }, []);

    return (
        <div className="glass" style={{
            padding: '12px 20px',
            marginBottom: '16px',
            display: 'flex',
            alignItems: 'center',
            justifyContent: 'space-between',
            background: 'rgba(20, 22, 30, 0.8)',
            border: '1px solid rgba(0, 243, 255, 0.3)'
        }}>
            <div style={{ display: 'flex', flexDirection: 'column' }}>
                <span style={{
                    fontSize: '0.8rem',
                    color: 'var(--text-secondary)',
                    textTransform: 'uppercase',
                    letterSpacing: '1px'
                }}>Active Context</span>
                <span style={{
                    fontSize: '1.2rem',
                    fontWeight: 'bold',
                    color: context.activeApp === 'CHROME' ? 'var(--accent-purple)' : 'var(--accent-cyan)'
                }}>
                    {context.activeApp}
                </span>
            </div>

            <div style={{ display: 'flex', alignItems: 'center', gap: '15px' }}>
                <div className={`macro-status-badge ${context.numLockActive ? 'disabled' : 'enabled'}`}>
                    <div className="status-dot"></div>
                    <span>Macros: {context.numLockActive ? 'DISABLED' : 'ENABLED'}</span>
                </div>
            </div>

            <div style={{ textAlign: 'right', maxWidth: '60%' }}>
                <div style={{ fontSize: '0.9rem', whiteSpace: 'nowrap', overflow: 'hidden', textOverflow: 'ellipsis' }}>
                    {context.activeTitle || "(No Title)"}
                </div>
                <div style={{ fontSize: '0.8rem', color: 'var(--text-secondary)', whiteSpace: 'nowrap', overflow: 'hidden', textOverflow: 'ellipsis' }}>
                    {context.activeUrl || "..."}
                </div>
            </div>
        </div>
    );
};

export default ContextMonitor;
