import { useState } from 'react';
import type { KeyAction, MacrosByApp } from '../../types';
import { COMMON_KEYS } from '../../types';
import MacroBuilder from '../MacroBuilder';
import Button from '../UI/Button';
import Badge from '../UI/Badge';

interface ConfigsProps {
    macros: MacrosByApp;
    onSave: (macros: MacrosByApp) => void;
}

const Configs = ({ macros, onSave }: ConfigsProps) => {
    const [editingMacro, setEditingMacro] = useState<{ app: string, action: KeyAction, index: number } | null>(null);
    const [selectedApp, setSelectedApp] = useState<string>('CHROME');

    const handleSaveMacro = (newAction: KeyAction) => {
        if (!editingMacro) return;

        const newMacros = { ...macros };
        const appMacros = [...(newMacros[editingMacro.app] || [])];

        if (editingMacro.index === -1) {
            appMacros.push(newAction);
        } else {
            appMacros[editingMacro.index] = newAction;
        }

        newMacros[editingMacro.app] = appMacros;
        onSave(newMacros);
        setEditingMacro(null);
    };

    if (editingMacro) {
        return (
            <MacroBuilder
                initialAction={editingMacro.index !== -1 ? editingMacro.action : undefined}
                onSave={handleSaveMacro}
                onCancel={() => setEditingMacro(null)}
            />
        );
    }

    const availableApps = ['CHROME', 'TERMINAL', 'CODE', 'OTHER'];

    return (
        <div className="configs-container">
            <div className="panel-header" style={{ marginBottom: '30px', border: 'none', padding: 0 }}>
                <div>
                    <h2 style={{ fontSize: '1.5rem', marginBottom: '8px' }}>Macro Configurations</h2>
                    <p style={{ color: 'var(--text-dim)', fontSize: '0.9rem' }}>
                        Manage triggers and sequences for your favorite applications.
                    </p>
                </div>
                <Button variant="primary" onClick={() => setEditingMacro({
                    app: selectedApp,
                    action: {
                        logMessage: 'New Macro',
                        trigger: { keys: [], hasSuppressedKeys: false },
                        sequence: [],
                        hasHandler: false
                    },
                    index: -1
                })}>
                    + Create Macro
                </Button>
            </div>

            <div className="app-tabs">
                {availableApps.map(app => (
                    <button
                        key={app}
                        className={`app-tab ${selectedApp === app ? 'active' : ''}`}
                        onClick={() => setSelectedApp(app)}
                    >
                        {app}
                    </button>
                ))}
            </div>

            <div className="macro-list">
                {(macros[selectedApp] || []).map((action, idx) => (
                    <div key={idx} className="macro-card glass">
                        <div className="macro-header">
                            <div>
                                <div className="macro-title">{action.logMessage || 'Unnamed Macro'}</div>
                                {action.hasHandler && (
                                    <Badge variant="primary" style={{ marginTop: '4px' }}>System Callback</Badge>
                                )}
                            </div>
                            <Button variant="secondary" onClick={() => setEditingMacro({ app: selectedApp, action, index: idx })}>
                                Edit
                            </Button>
                        </div>

                        <div style={{ display: 'flex', flexDirection: 'column', gap: '12px' }}>
                            <div>
                                <div style={{ fontSize: '0.75rem', color: 'var(--text-dim)', textTransform: 'uppercase', marginBottom: '8px', fontWeight: '700' }}>Trigger Conditions</div>
                                <div className="trigger-details">
                                    {action.trigger?.keys?.map((k, kIdx) => (
                                        <div key={kIdx} style={{
                                            display: 'flex',
                                            flexDirection: 'column',
                                            gap: '4px',
                                            padding: '8px 12px',
                                            background: 'rgba(255,255,255,0.03)',
                                            borderRadius: '8px',
                                            border: '1px solid var(--glass-border)'
                                        }}>
                                            <span style={{ fontWeight: '700', color: 'var(--accent-cyan)' }}>
                                                {COMMON_KEYS[k.code] || `Key ${k.code}`}
                                            </span>
                                            <div style={{ display: 'flex', gap: '4px' }}>
                                                <Badge variant={k.state === 1 ? 'success' : 'default'} style={{ fontSize: '0.65rem' }}>
                                                    {k.state === 1 ? 'PRESS' : 'RELEASE'}
                                                </Badge>
                                                {k.suppress && <Badge variant="warning" style={{ fontSize: '0.65rem' }}>WITHHOLD</Badge>}
                                                {k.ignoreRepeat && <Badge variant="danger" style={{ fontSize: '0.65rem' }}>BREAKS</Badge>}
                                            </div>
                                        </div>
                                    ))}
                                </div>
                            </div>

                            {action.sequence.length > 0 && (
                                <div>
                                    <div style={{ fontSize: '0.75rem', color: 'var(--text-dim)', textTransform: 'uppercase', marginBottom: '8px', fontWeight: '700' }}>Output Sequence</div>
                                    <div style={{ display: 'flex', flexWrap: 'wrap', gap: '4px', fontFamily: 'JetBrains Mono, monospace', fontSize: '0.8rem' }}>
                                        {action.sequence.map((s, sIdx) => (
                                            <span key={sIdx} style={{ color: s.value === 1 ? 'var(--accent-cyan)' : 'var(--text-dim)' }}>
                                                {COMMON_KEYS[s.code] || s.code}{sIdx < action.sequence.length - 1 ? ' → ' : ''}
                                            </span>
                                        ))}
                                    </div>
                                </div>
                            )}
                        </div>
                    </div>
                ))}

                {(macros[selectedApp] || []).length === 0 && (
                    <div style={{
                        gridColumn: '1 / -1',
                        padding: '60px',
                        textAlign: 'center',
                        color: 'var(--text-dim)',
                        border: '2px dashed var(--glass-border)',
                        borderRadius: '16px'
                    }}>
                        <div style={{ fontSize: '3rem', marginBottom: '16px' }}>⚡</div>
                        <h3 style={{ color: 'var(--text-secondary)' }}>No macros found for {selectedApp}</h3>
                        <p>Get started by creating your first macro for this application.</p>
                    </div>
                )}
            </div>
        </div>
    );
};

export default Configs;
