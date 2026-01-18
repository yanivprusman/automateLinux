import { useEffect, useRef, useState, type KeyboardEvent } from 'react';
import Button from '../UI/Button';
import Input from '../UI/Input';
import Badge from '../UI/Badge';
import Toggle from '../UI/Toggle';

interface LiveLogsProps {
    logs: string[];
    filters: string[];
    onSetFilters: (filters: string[]) => void;
    onClearLogs: () => void;
    isLoggingEnabled: boolean;
    onToggleLogging: (enabled: boolean) => void;
}

const LiveLogs = ({ logs, filters, onSetFilters, onClearLogs, isLoggingEnabled, onToggleLogging }: LiveLogsProps) => {
    const logEndRef = useRef<HTMLDivElement>(null);
    const [autoScroll, setAutoScroll] = useState(true);
    const [newPattern, setNewPattern] = useState('');

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

    const handleAddPattern = () => {
        if (newPattern && !filters.includes(newPattern)) {
            onSetFilters([...filters, newPattern]);
            setNewPattern('');
        }
    };

    const handleKeyPress = (e: KeyboardEvent<HTMLInputElement>) => {
        if (e.key === 'Enter') {
            handleAddPattern();
        }
    };

    const removeFilter = (pattern: string) => {
        onSetFilters(filters.filter(f => f !== pattern));
    };

    const presets = [
        { label: 'All Keys', pattern: 'key' },
        { label: 'All Buttons', pattern: 'btn' },
        { label: 'All Misc', pattern: 'msc' },
        { label: 'Down Only', pattern: '::1' },
        { label: 'Up Only', pattern: '::0' },
        { label: 'Repeat Only', pattern: '::2' },
    ];

    return (
        <div className="live-logs-container">
            <div className="panel-header glass" style={{ borderBottom: 'none', marginBottom: '16px' }}>
                <div>
                    <h2 style={{ fontSize: '1.2rem' }}>Live System Monitor</h2>
                    <p style={{ color: 'var(--text-dim)', fontSize: '0.8rem', marginTop: '4px' }}>
                        Real-time input events and macro execution traces.
                    </p>
                </div>
                <div style={{ display: 'flex', gap: '20px', alignItems: 'center' }}>
                    <Toggle
                        label="Live Logging"
                        checked={isLoggingEnabled}
                        onChange={onToggleLogging}
                        size="small"
                    />
                    <div style={{ width: '1px', height: '24px', background: 'var(--glass-border)' }} />
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
                    <div style={{ display: 'flex', height: '100%', alignItems: 'center', justifyContent: 'center', color: 'var(--text-dim)', fontStyle: 'italic', textAlign: 'center', padding: '0 20px' }}>
                        {isLoggingEnabled
                            ? 'Waiting for events...'
                            : 'Logging is disabled. Enable "Live Logging" above to see real-time events.'}
                    </div>
                )}
            </div>

            <div className="filters-panel glass" style={{ marginTop: '16px', border: 'none' }}>
                <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', marginBottom: '16px' }}>
                    <h3 style={{ fontSize: '0.85rem', fontWeight: '700', color: 'var(--text-secondary)', textTransform: 'uppercase', margin: 0 }}>
                        Event Isolation Filters
                    </h3>
                    <div style={{ display: 'flex', gap: '8px' }}>
                        {filters.length > 0 && (
                            <Button size="small" variant="secondary" onClick={() => onSetFilters([])} style={{ fontSize: '0.65rem', padding: '2px 8px' }}>
                                Clear All
                            </Button>
                        )}
                        <span style={{ fontSize: '0.75rem', color: 'var(--text-dim)' }}>
                            {filters.length} active criteria
                        </span>
                    </div>
                </div>

                <div style={{ display: 'flex', flexDirection: 'column', gap: '16px' }}>
                    <div style={{ display: 'flex', gap: '12px', alignItems: 'flex-start' }}>
                        <div style={{ flex: 1 }}>
                            <Input
                                placeholder="Add pattern (e.g. key:a, ::1, btn:left...)"
                                value={newPattern}
                                onChange={(e) => setNewPattern(e.target.value)}
                                onKeyPress={handleKeyPress}
                                style={{ height: '40px' }}
                            />
                        </div>
                        <Button variant="primary" onClick={handleAddPattern} style={{ height: '40px' }}>
                            Add Filter
                        </Button>
                    </div>

                    <div style={{ display: 'flex', flexWrap: 'wrap', gap: '8px' }}>
                        <span style={{ fontSize: '0.75rem', color: 'var(--text-dim)', alignSelf: 'center', marginRight: '4px' }}>PRESETS:</span>
                        {presets.map(p => (
                            <button
                                key={p.pattern}
                                className={`filter-chip ${filters.includes(p.pattern) ? 'active' : ''}`}
                                onClick={() => filters.includes(p.pattern) ? removeFilter(p.pattern) : onSetFilters([...filters, p.pattern])}
                                style={{ fontSize: '0.7rem', padding: '4px 10px' }}
                            >
                                {p.label}
                            </button>
                        ))}
                    </div>

                    {filters.length > 0 && (
                        <div style={{ display: 'flex', flexWrap: 'wrap', gap: '8px', padding: '12px', background: 'rgba(255,255,255,0.02)', borderRadius: '8px', border: '1px solid var(--glass-border)' }}>
                            {filters.map(f => (
                                <Badge
                                    key={f}
                                    variant="primary"
                                    style={{
                                        display: 'flex',
                                        alignItems: 'center',
                                        gap: '6px',
                                        padding: '4px 8px',
                                        cursor: 'default'
                                    }}
                                >
                                    {f}
                                    <span
                                        onClick={() => removeFilter(f)}
                                        style={{
                                            cursor: 'pointer',
                                            opacity: 0.6,
                                            fontSize: '1rem',
                                            lineHeight: 1,
                                            marginLeft: '4px',
                                            display: 'inline-block'
                                        }}
                                        title="Remove filter"
                                    >
                                        ×
                                    </span>
                                </Badge>
                            ))}
                        </div>
                    )}
                </div>
            </div>
        </div>
    );
};

export default LiveLogs;
