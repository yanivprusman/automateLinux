import { useEffect, useState } from 'react';

interface ContextData {
    activeApp: string;
    activeUrl: string;
    activeTitle: string;
    numLockActive: boolean;
}

const ContextMonitor = () => {
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

        fetchContext();
        const interval = setInterval(fetchContext, 1000);
        return () => clearInterval(interval);
    }, []);

    return (
        <header className="context-monitor glass">
            <div style={{ display: 'flex', alignItems: 'center', gap: '20px' }}>
                <div style={{ display: 'flex', flexDirection: 'column' }}>
                    <span style={{
                        fontSize: '0.7rem',
                        color: 'var(--text-dim)',
                        textTransform: 'uppercase',
                        letterSpacing: '1.5px',
                        fontWeight: '700',
                        marginBottom: '2px'
                    }}>Active Context</span>
                    <span style={{
                        fontSize: '1.1rem',
                        fontWeight: '800',
                        color: context.activeApp === 'CHROME' ? 'var(--accent-purple)' : 'var(--accent-cyan)',
                        letterSpacing: '0.5px'
                    }}>
                        {context.activeApp}
                    </span>
                </div>

                <div className={`macro-status-badge ${context.numLockActive ? 'disabled' : 'enabled'}`}>
                    <div className="status-dot"></div>
                    <span>Macros: {context.numLockActive ? 'OFF' : 'ON'}</span>
                </div>
            </div>

            <div style={{ textAlign: 'right', flex: 1, marginLeft: '40px', overflow: 'hidden' }}>
                <div style={{
                    fontSize: '0.95rem',
                    fontWeight: '600',
                    whiteSpace: 'nowrap',
                    overflow: 'hidden',
                    textOverflow: 'ellipsis',
                    color: 'var(--text-primary)'
                }}>
                    {context.activeTitle || "System Idle"}
                </div>
                <div style={{
                    fontSize: '0.8rem',
                    color: 'var(--text-dim)',
                    whiteSpace: 'nowrap',
                    overflow: 'hidden',
                    textOverflow: 'ellipsis',
                    fontFamily: 'JetBrains Mono, monospace'
                }}>
                    {context.activeUrl || "Waiting for interaction..."}
                </div>
            </div>
        </header>
    );
};

export default ContextMonitor;
