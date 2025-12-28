import React, { useEffect, useRef, useState } from 'react';
import { COMMON_KEYS } from '../../types';

interface LiveLogsProps {
    logs: string[];
    filters: number[];
    onToggleFilter: (code: number) => void;
    onClearLogs: () => void;
}

const LiveLogs: React.FC<LiveLogsProps> = ({ logs, filters, onToggleFilter, onClearLogs }) => {
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
        <div className="live-logs-container glass">
            <div className="panel-header">
                <h2>Live Event Stream</h2>
                <div className="header-actions">
                    <button className="btn-secondary" onClick={onClearLogs}>Clear</button>
                    <label className="auto-scroll-toggle">
                        <input type="checkbox" checked={autoScroll} onChange={(e) => setAutoScroll(e.target.checked)} />
                        Auto-scroll
                    </label>
                </div>
            </div>

            <div className="logs-viewer" onScroll={onScroll}>
                {logs.map((log, i) => (
                    <div key={i} className="log-entry">
                        <span className="log-timestamp">{new Date().toLocaleTimeString()}</span>
                        <span className="log-content">{log}</span>
                    </div>
                ))}
                <div ref={logEndRef} />
            </div>

            <div className="filters-panel glass">
                <h3>Event Filters</h3>
                <div className="chips-grid">
                    {Object.entries(COMMON_KEYS).map(([code, name]) => (
                        <label key={code} className={`filter-chip ${filters.includes(Number(code)) ? 'active' : ''}`}>
                            <input
                                type="checkbox"
                                checked={filters.includes(Number(code))}
                                onChange={() => onToggleFilter(Number(code))}
                            />
                            {name} ({code})
                        </label>
                    ))}
                </div>
            </div>
        </div>
    );
};

export default LiveLogs;
