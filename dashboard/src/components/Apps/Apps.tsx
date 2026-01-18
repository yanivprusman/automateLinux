import { useState } from 'react';
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

const Apps = () => {
    const [loading, setLoading] = useState<string | null>(null);

    const launchers: AppLauncher[] = [
        {
            id: 'loom',
            label: 'Loom',
            icon: '📹',
            command: 'restartLoom',
            description: 'Restart Loom Server & Client',
        },
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

    return (
        <div className="apps-container">
            <h1>Applications</h1>
            <div className="apps-grid">
                {launchers.map(app => (
                    <div key={app.id} className="app-card glass">
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
                                        onClick={() => handleLaunch(app.id, opt.command, opt.args, opt.url)}
                                    >
                                        {opt.label}
                                    </button>
                                ))}
                            </div>
                        ) : (
                            <button
                                className="launch-btn"
                                disabled={loading === app.id}
                                onClick={() => handleLaunch(app.id, app.command, app.args, app.url)}
                            >
                                {loading === app.id ? 'Launching...' : 'Launch'}
                            </button>
                        )}
                    </div>
                ))}
            </div>
        </div>
    );
};

export default Apps;
