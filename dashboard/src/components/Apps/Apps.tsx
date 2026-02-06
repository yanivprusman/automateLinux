import { useState } from 'react';
import type { Peer } from '../../peers';
import './Apps.css';

interface AppLauncher {
    id: string;
    label: string;
    icon: string;
    description: string;
    command?: string;
    args?: any;
    url?: string;
    options?: {
        label: string;
        url?: string;
        command?: string;
        args?: any;
    }[];
}

interface PeerAppStatus {
    peer_id: string;
    is_online: boolean;
    dev_installed: boolean | null;
    prod_installed: boolean | null;
    dev_running: boolean | null;
    prod_running: boolean | null;
    error: string | null;
}

interface AppPeerInfo {
    app: string;
    peers: PeerAppStatus[];
}

interface Props {
    selectedPeer: Peer;
}

const Apps = ({ selectedPeer }: Props) => {
    const [loading, setLoading] = useState<string | null>(null);
    const [expandedApp, setExpandedApp] = useState<string | null>(null);
    const [peerData, setPeerData] = useState<Record<string, AppPeerInfo>>({});
    const [peerLoading, setPeerLoading] = useState<string | null>(null);

    const launchers: AppLauncher[] = [
        {
            id: 'pt',
            label: 'Public Transport',
            icon: '🚌',
            command: 'publicTransportationOpenApp',
            description: 'Open Public Transportation App',
            options: [
                { label: 'Prod', command: 'publicTransportationOpenApp', args: { variant: 'prod' } },
                { label: 'Dev', command: 'publicTransportationOpenApp', args: { variant: 'dev' } },
            ]
        },
        {
            id: 'cad',
            label: 'Calculated CAD',
            icon: '📐',
            description: 'Open CAD Application',
            options: [
                { label: 'Prod', url: 'http://localhost:3000' },
                { label: 'Dev', url: 'http://localhost:3001' },
                { label: 'Debug', url: 'http://localhost:3001/?rawPart=Right%20Wall%20-%20Panel%201' },
            ]
        },
    ];

    const handleLaunch = async (appId: string, command?: string, args?: any, url?: string) => {
        if (url) {
            window.open(url, '_blank');
            return;
        }

        if (!command) return;

        setLoading(appId);
        try {
            await fetch('http://localhost:3501/api/command', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({
                    command: command,
                    ...args
                })
            });
        } catch (err) {
            console.error(err);
            alert('Failed to launch app');
        } finally {
            setLoading(null);
        }
    };

    const fetchPeerStatus = async (appId: string) => {
        setPeerLoading(appId);
        try {
            const res = await fetch(`http://localhost:3501/api/app-peers/${appId}`);
            const data: AppPeerInfo = await res.json();
            setPeerData(prev => ({ ...prev, [appId]: data }));
        } catch (err) {
            console.error('Failed to fetch peer status:', err);
        } finally {
            setPeerLoading(null);
        }
    };

    const handleCardClick = (appId: string) => {
        if (expandedApp === appId) {
            setExpandedApp(null);
        } else {
            setExpandedApp(appId);
            fetchPeerStatus(appId);
        }
    };

    const renderStatusBadge = (label: string, value: boolean | null, type: 'installed' | 'running') => {
        if (value === null) return <span className="status-badge unknown">{label}: ?</span>;
        const className = value
            ? (type === 'running' ? 'status-badge running' : 'status-badge installed')
            : (type === 'running' ? 'status-badge stopped' : 'status-badge not-installed');
        const text = type === 'running'
            ? (value ? 'Running' : 'Stopped')
            : (value ? 'Installed' : 'Not installed');
        return <span className={className}>{label}: {text}</span>;
    };

    return (
        <div className="apps-container">
            <h1>Applications</h1>
            <div className="apps-grid">
                {launchers.map(app => (
                    <div
                        key={app.id}
                        className={`app-card glass${expandedApp === app.id ? ' expanded' : ''}`}
                        onClick={() => handleCardClick(app.id)}
                    >
                        <div className="app-card-header">
                            <div className="app-icon">{app.icon}</div>
                            <h3>{app.label}</h3>
                            <p>{app.description}</p>

                            {app.options ? (
                                <div className="button-group">
                                    {app.options.map((opt, idx) => (
                                        <button
                                            key={idx}
                                            className="launch-btn small"
                                            disabled={loading === app.id}
                                            onClick={(e) => {
                                                e.stopPropagation();
                                                handleLaunch(app.id, opt.command, opt.args, opt.url);
                                            }}
                                        >
                                            {opt.label}
                                        </button>
                                    ))}
                                </div>
                            ) : (
                                <button
                                    className="launch-btn"
                                    disabled={loading === app.id}
                                    onClick={(e) => {
                                        e.stopPropagation();
                                        handleLaunch(app.id, app.command, app.args, app.url);
                                    }}
                                >
                                    {loading === app.id ? 'Launching...' : 'Launch'}
                                </button>
                            )}
                        </div>

                        {expandedApp === app.id && (
                            <div className="peer-status-section">
                                <div className="peer-status-header">
                                    <span>Peer Status</span>
                                    <button
                                        className="refresh-btn"
                                        onClick={(e) => {
                                            e.stopPropagation();
                                            fetchPeerStatus(app.id);
                                        }}
                                        disabled={peerLoading === app.id}
                                    >
                                        {peerLoading === app.id ? 'Loading...' : 'Refresh'}
                                    </button>
                                </div>
                                {peerData[app.id] ? (
                                    <div className="peer-rows">
                                        {peerData[app.id].peers.map(peer => (
                                            <div key={peer.peer_id} className={`peer-row${!peer.is_online ? ' offline' : ''}`}>
                                                <span className="peer-name">{peer.peer_id}</span>
                                                {peer.error ? (
                                                    <span className="status-badge error">{peer.error}</span>
                                                ) : (
                                                    <div className="peer-badges">
                                                        {renderStatusBadge('Dev', peer.dev_installed, 'installed')}
                                                        {renderStatusBadge('Dev', peer.dev_running, 'running')}
                                                        {renderStatusBadge('Prod', peer.prod_installed, 'installed')}
                                                        {renderStatusBadge('Prod', peer.prod_running, 'running')}
                                                    </div>
                                                )}
                                            </div>
                                        ))}
                                    </div>
                                ) : peerLoading === app.id ? (
                                    <div className="peer-loading">Querying peers...</div>
                                ) : null}
                            </div>
                        )}
                    </div>
                ))}
            </div>
        </div>
    );
};

export default Apps;
