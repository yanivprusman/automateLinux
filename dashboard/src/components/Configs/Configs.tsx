import React, { useState } from 'react';
import type { KeyAction, MacrosByApp } from '../../types';
import MacroBuilder from '../MacroBuilder';

interface ConfigsProps {
    macros: MacrosByApp;
    onSave: (macros: MacrosByApp) => void;
}

const Configs: React.FC<ConfigsProps> = ({ macros, onSave }) => {
    // Flatten macros to a list for easier editing
    const [editingMacro, setEditingMacro] = useState<{ app: string, action: KeyAction, index: number } | null>(null);
    const [selectedApp, setSelectedApp] = useState<string>('CHROME'); // Default

    const handleSaveMacro = (newAction: KeyAction) => {
        if (!editingMacro) return;

        const newMacros = { ...macros };
        const appMacros = [...(newMacros[editingMacro.app] || [])];

        if (editingMacro.index === -1) {
            appMacros.push(newAction); // Add new
        } else {
            appMacros[editingMacro.index] = newAction; // Update
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

    const availableApps = Object.keys(macros).length > 0 ? Object.keys(macros) : ['CHROME', 'TERMINAL', 'CODE'];

    return (
        <div className="configs-container glass">
            <div className="panel-header" style={{ marginBottom: '20px' }}>
                <h2>Macro Configurations</h2>
                <div className="app-tabs" style={{ display: 'flex', gap: '10px', marginTop: '10px' }}>
                    {availableApps.map(app => (
                        <button
                            key={app}
                            className={`glass-button ${selectedApp === app ? 'primary' : ''}`}
                            onClick={() => setSelectedApp(app)}
                        >
                            {app}
                        </button>
                    ))}
                </div>
            </div>

            <div className="macro-list" style={{ display: 'flex', flexDirection: 'column', gap: '10px' }}>
                {(macros[selectedApp] || []).map((action, idx) => (
                    <div key={idx} className="macro-item glass" style={{
                        padding: '15px',
                        display: 'flex',
                        justifyContent: 'space-between',
                        alignItems: 'center',
                        background: 'rgba(255,255,255,0.03)'
                    }}>
                        <div>
                            <div style={{ fontWeight: 'bold' }}>{action.logMessage || 'Unnamed Macro'}</div>
                            <div style={{ fontSize: '0.8rem', color: 'var(--text-secondary)' }}>
                                Trigger: {action.trigger?.keys?.map(k => k.code).join(' + ')}
                            </div>
                        </div>
                        <button
                            className="glass-button secondary"
                            onClick={() => setEditingMacro({ app: selectedApp, action, index: idx })}
                        >
                            Edit
                        </button>
                    </div>
                ))}

                {(macros[selectedApp] || []).length === 0 && (
                    <div style={{ padding: '20px', textAlign: 'center', color: 'var(--text-secondary)' }}>
                        No macros configured for {selectedApp}
                    </div>
                )}

                <button
                    className="glass-button primary"
                    style={{ marginTop: '10px', alignSelf: 'flex-start' }}
                    onClick={() => setEditingMacro({
                        app: selectedApp,
                        action: {
                            logMessage: 'New Macro',
                            trigger: { keys: [], hasSuppressedKeys: false },
                            sequence: [],
                            hasHandler: false
                        },
                        index: -1
                    })}
                >
                    + Add New Macro
                </button>
            </div>
        </div>
    );
};

export default Configs;
