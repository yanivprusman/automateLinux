import { useEffect, useRef, useState } from 'react';
import { COMMON_KEYS } from '../../types';
import Button from '../UI/Button';

interface LiveLogsProps {
    logs: string[];
    filters: number[];
    onToggleFilter: (code: number) => void;
    onClearLogs: () => void;
}

const LiveLogs = ({ logs, filters, onToggleFilter, onClearLogs }: LiveLogsProps) => {
    const logEndRef = useRef<HTMLDivElement>(null);
    const [autoScroll, setAutoScroll] = useState(true);

    useEffect(() => {
        if (autoScroll) {
            logEndRef.current?.scrollIntoView({ behavior: 'smooth' });
        }
    }, [logs, autoScroll]);

    const onScroll = (e: React.UIEvent<HTMLDivElement>) => {
        const { scrollTop, scrollHeight, clientHeight } = e.currentTarget;
        const isAtBottom = scrollHeight - scrollTop - clientHeight < 50;
        setAutoScroll(isAtBottom);
    };

    return (
        <div className="live-logs-container">
            <div className="panel-header glass" style={{ borderBottom: 'none', marginBottom: '16px' }}>
                <div>
                    <h2 style={{ fontSize: '1.2rem' }}>Live System Monitor</h2>
                    <p style={{ color: 'var(--text-dim)', fontSize: '0.8rem', marginTop: '4px' }}>
                        Real-time input events and macro execution traces.
                    </p>
                </div>
                <div style={{ display: 'flex', gap: '12px', alignItems: 'center' }}>
                    <Button
                        size="small"
                        variant={autoScroll ? 'primary' : 'default'}
                        onClick={() => setAutoScroll(!autoScroll)}
                        style={{ fontSize: '0.75rem' }}
                    >
                        {autoScroll ? 'AUTO-SCROLL ON' : 'AUTO-SCROLL OFF'}
                    </Button>
                    <Button size="small" onClick={onClearLogs} style={{ fontSize: '0.75rem' }}>
                        Clear Logs
                    </Button>
                </div>
            </div>

            <div className="logs-viewer glass" onScroll={onScroll}>
                {logs.map((log, i) => (
                    <div key={i} className="log-entry">
                        <span className="log-timestamp" style={{ fontSize: '0.75rem' }}>
                            [{new Date().toLocaleTimeString('en-US', { hour12: false, fractionDigits: 3 } as any)}]
                        </span>
                        <span className="log-content">{log}</span>
                    </div>
                ))}
                <div ref={logEndRef} />
                {logs.length === 0 && (
                    <div style={{ display: 'flex', height: '100%', alignItems: 'center', justifyContent: 'center', color: 'var(--text-dim)', fontStyle: 'italic' }}>
                        Waiting for events...
                    </div>
                )}
            </div>

            <div className="filters-panel glass" style={{ marginTop: '16px', border: 'none' }}>
                <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', marginBottom: '16px' }}>
                    <h3 style={{ fontSize: '0.85rem', fontWeight: '700', color: 'var(--text-secondary)', textTransform: 'uppercase', margin: 0 }}>
                        Event Isolation Filters
                    </h3>
                    <span style={{ fontSize: '0.75rem', color: 'var(--text-dim)' }}>
                        {filters.length} active filters
                    </span>
                </div>

                <div className="chips-grid">
                    {/* Common filters first */}
                    {[1, 28, 57, 272, 273, 275, 277].map(code => (
                        <div
                            key={code}
                            className={`filter-chip ${filters.includes(code) ? 'active' : ''}`}
                            onClick={() => onToggleFilter(code)}
                        >
                            {COMMON_KEYS[code]}
                        </div>
                    ))}
                    <div style={{ width: '1px', background: 'var(--glass-border)', margin: '0 8px' }} />
                    {/* Others */}
                    {Object.entries(COMMON_KEYS)
                        .filter(([code]) => ![1, 28, 57, 272, 273, 275, 277].includes(Number(code)))
                        .slice(0, 15) // Limit viewable filters
                        .map(([code, name]) => (
                            <div
                                key={code}
                                className={`filter-chip ${filters.includes(Number(code)) ? 'active' : ''}`}
                                onClick={() => onToggleFilter(Number(code))}
                            >
                                {name}
                            </div>
                        ))
                    }
                </div>
            </div>
        </div>
    );
};

export default LiveLogs;
