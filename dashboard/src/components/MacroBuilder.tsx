import React, { useState } from 'react';
import type { KeyAction, KeyTrigger } from '../types';
import { COMMON_KEYS } from '../types';

interface MacroBuilderProps {
    initialAction?: KeyAction;
    onSave: (action: KeyAction) => void;
    onCancel: () => void;
}

const MacroBuilder: React.FC<MacroBuilderProps> = ({ initialAction, onSave, onCancel }) => {
    const [logMessage, setLogMessage] = useState(initialAction?.logMessage || '');
    const [triggerKeys, setTriggerKeys] = useState<number[]>(
        initialAction?.trigger?.keys?.map(k => k.code) || []
    );
    const [sequence, setSequence] = useState<{ code: number, value: number }[]>(
        initialAction?.sequence || []
    );
    const [isGKeyMode, setIsGKeyMode] = useState(
        initialAction?.trigger?.keys?.length === 1 &&
        initialAction.trigger.keys[0].code >= 1001 &&
        initialAction.trigger.keys[0].code <= 1006
    );

    const availableGKeys = [
        { code: 1001, name: 'G1' },
        { code: 1002, name: 'G2' },
        { code: 1003, name: 'G3' },
        { code: 1004, name: 'G4' },
        { code: 1005, name: 'G5' },
        { code: 1006, name: 'G6' },
    ];

    const handleSave = () => {
        const newTrigger: KeyTrigger = {
            keys: triggerKeys.map(code => ({
                code,
                state: 1, // Default to press
                suppress: true // Default to suppress
            })),
            hasSuppressedKeys: true
        };

        const newAction: KeyAction = {
            trigger: newTrigger,
            sequence: sequence,
            logMessage: logMessage || 'Untitled Macro',
            hasHandler: false // handlers are special cased for now
        };

        onSave(newAction);
    };

    const addSequenceStep = () => {
        // Default to A key press (30)
        setSequence([...sequence, { code: 30, value: 1 }, { code: 30, value: 0 }]);
    };

    const updateSequenceStep = (index: number, field: 'code' | 'value', val: number) => {
        const newSeq = [...sequence];
        newSeq[index] = { ...newSeq[index], [field]: val };
        setSequence(newSeq);
    };

    const removeSequenceStep = (index: number) => {
        setSequence(sequence.filter((_, i) => i !== index));
    };

    return (
        <div className="macro-builder glass" style={{ padding: '20px' }}>
            <h3>{initialAction ? 'Edit Macro' : 'New Macro'}</h3>

            <div className="form-group" style={{ marginBottom: '15px' }}>
                <label>Name / Log Message</label>
                <input
                    type="text"
                    value={logMessage}
                    onChange={e => setLogMessage(e.target.value)}
                    className="glass-input"
                    style={{ width: '100%', padding: '8px', marginTop: '5px' }}
                />
            </div>

            <div className="form-group" style={{ marginBottom: '15px' }}>
                <label>Trigger</label>
                <div style={{ margin: '10px 0' }}>
                    <label style={{ marginRight: '15px' }}>
                        <input
                            type="radio"
                            checked={!isGKeyMode}
                            onChange={() => setIsGKeyMode(false)}
                        /> Key Sequence
                    </label>
                    <label>
                        <input
                            type="radio"
                            checked={isGKeyMode}
                            onChange={() => setIsGKeyMode(true)}
                        /> G-Key
                    </label>
                </div>

                {isGKeyMode ? (
                    <select
                        className="glass-input"
                        value={triggerKeys[0] || 1001}
                        onChange={e => setTriggerKeys([Number(e.target.value)])}
                    >
                        {availableGKeys.map(gk => (
                            <option key={gk.code} value={gk.code}>{gk.name}</option>
                        ))}
                    </select>
                ) : (
                    <div style={{ display: 'flex', gap: '5px' }}>
                        {triggerKeys.map((code, idx) => (
                            <div key={idx} className="key-badge" style={{
                                background: '#333', padding: '4px 8px', borderRadius: '4px', display: 'flex', alignItems: 'center', gap: '5px'
                            }}>
                                {COMMON_KEYS[code] || code}
                                <button onClick={() => setTriggerKeys(triggerKeys.filter((_, i) => i !== idx))} style={{ border: 'none', background: 'transparent', color: 'red' }}>×</button>
                            </div>
                        ))}
                        <div style={{ display: 'flex', gap: '5px' }}>
                            <input
                                type="number"
                                placeholder="Code"
                                className="glass-input"
                                style={{ width: '60px' }}
                                onKeyDown={e => {
                                    if (e.key === 'Enter') {
                                        setTriggerKeys([...triggerKeys, Number(e.currentTarget.value)]);
                                        e.currentTarget.value = '';
                                    }
                                }}
                            />
                            <button className="glass-button secondary">Add</button>
                        </div>
                    </div>
                )}
            </div>

            <div className="form-group" style={{ marginBottom: '15px' }}>
                <label>Output Sequence</label>
                <div className="sequence-list" style={{ maxHeight: '200px', overflowY: 'auto', background: 'rgba(0,0,0,0.2)', borderRadius: '4px', padding: '10px' }}>
                    {sequence.map((step, idx) => (
                        <div key={idx} className="sequence-step" style={{ display: 'flex', gap: '10px', marginBottom: '5px', alignItems: 'center' }}>
                            <span style={{ color: '#888', width: '20px' }}>{idx + 1}.</span>
                            <select
                                className="glass-input"
                                value={step.code}
                                onChange={e => updateSequenceStep(idx, 'code', Number(e.target.value))}
                                style={{ width: '120px' }}
                            >
                                <option value={step.code}>{COMMON_KEYS[step.code] || `Key ${step.code}`}</option>
                                {Object.entries(COMMON_KEYS).map(([c, n]) => (
                                    <option key={c} value={c}>{n}</option>
                                ))}
                            </select>
                            <select
                                className="glass-input"
                                value={step.value}
                                onChange={e => updateSequenceStep(idx, 'value', Number(e.target.value))}
                            >
                                <option value={1}>Press</option>
                                <option value={0}>Release</option>
                            </select>
                            <button onClick={() => removeSequenceStep(idx)} className="glass-button secondary" style={{ color: 'red' }}>×</button>
                        </div>
                    ))}
                </div>
                <button onClick={addSequenceStep} className="glass-button secondary" style={{ marginTop: '5px' }}>+ Add Key Press</button>
            </div>

            <div className="actions" style={{ display: 'flex', gap: '10px', justifyContent: 'flex-end', marginTop: '20px' }}>
                <button onClick={onCancel} className="glass-button">Cancel</button>
                <button onClick={handleSave} className="glass-button primary">Save Macro</button>
            </div>
        </div>
    );
};

export default MacroBuilder;
