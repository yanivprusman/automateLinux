import { useState } from 'react';
import type { KeyAction, KeyTrigger } from '../types';
import { COMMON_KEYS } from '../types';
import Button from './UI/Button';
import Input from './UI/Input';

interface MacroBuilderProps {
    initialAction?: KeyAction;
    onSave: (action: KeyAction) => void;
    onCancel: () => void;
}

const MacroBuilder = ({ initialAction, onSave, onCancel }: MacroBuilderProps) => {
    const [logMessage, setLogMessage] = useState(initialAction?.logMessage || '');
    const [triggerKeys, setTriggerKeys] = useState<KeyTrigger['keys']>(
        initialAction?.trigger?.keys || []
    );
    const [sequence, setSequence] = useState<{ code: number, value: number }[]>(
        initialAction?.sequence || []
    );

    const handleSave = () => {
        const newTrigger: KeyTrigger = {
            keys: triggerKeys,
            hasSuppressedKeys: triggerKeys.some(k => k.suppress)
        };

        const newAction: KeyAction = {
            trigger: newTrigger,
            sequence: sequence,
            logMessage: logMessage || 'Untitled Macro',
            hasHandler: initialAction?.hasHandler || false
        };

        onSave(newAction);
    };

    const addTriggerKey = (code: number) => {
        if (!code) return;
        setTriggerKeys([...triggerKeys, {
            code,
            state: 1, // Default to press
            suppress: true,
            ignoreRepeat: false
        }]);
    };

    const updateTriggerKey = (index: number, updates: Partial<KeyTrigger['keys'][0]>) => {
        const newKeys = [...triggerKeys];
        newKeys[index] = { ...newKeys[index], ...updates };
        setTriggerKeys(newKeys);
    };

    const addSequenceStep = () => {
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
        <div className="configs-container" style={{ padding: 0 }}>
            <div className="panel-header" style={{ marginBottom: '30px', border: 'none', padding: 0 }}>
                <div>
                    <h2 style={{ fontSize: '1.5rem', marginBottom: '8px' }}>
                        {initialAction ? 'Edit Macro' : 'Create New Macro'}
                    </h2>
                    <p style={{ color: 'var(--text-dim)', fontSize: '0.9rem' }}>
                        Configure triggers and output sequences with precision.
                    </p>
                </div>
                <div style={{ display: 'flex', gap: '12px' }}>
                    <Button onClick={onCancel}>Cancel</Button>
                    <Button variant="primary" onClick={handleSave}>Save Macro</Button>
                </div>
            </div>

            <div style={{ display: 'grid', gridTemplateColumns: '1fr 1fr', gap: '32px' }}>
                <div className="glass" style={{ padding: '24px' }}>
                    <Input
                        label="Macro Discovery Name"
                        placeholder="e.g., Terminal SIGINT Macro"
                        value={logMessage}
                        onChange={e => setLogMessage(e.target.value)}
                    />

                    <div style={{ marginTop: '24px' }}>
                        <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', marginBottom: '16px' }}>
                            <label style={{ fontSize: '0.85rem', fontWeight: '700', color: 'var(--text-secondary)', textTransform: 'uppercase' }}>
                                Trigger Keys
                            </label>

                            <select
                                className="glass-input"
                                style={{ padding: '4px 12px', fontSize: '0.8rem', borderRadius: '8px' }}
                                onChange={(e) => {
                                    addTriggerKey(Number(e.target.value));
                                    e.target.value = '';
                                }}
                            >
                                <option value="">+ Add Key...</option>
                                {Object.entries(COMMON_KEYS)
                                    .sort((a, b) => a[1].localeCompare(b[1]))
                                    .map(([code, name]) => (
                                        <option key={code} value={code}>{name}</option>
                                    ))
                                }
                            </select>
                        </div>

                        <div style={{ display: 'flex', flexDirection: 'column', gap: '12px' }}>
                            {triggerKeys.map((k, idx) => (
                                <div key={idx} className="glass" style={{
                                    padding: '16px',
                                    background: 'rgba(255,255,255,0.02)',
                                    display: 'flex',
                                    flexDirection: 'column',
                                    gap: '12px'
                                }}>
                                    <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center' }}>
                                        <span style={{ fontWeight: '700', color: 'var(--accent-cyan)' }}>
                                            {COMMON_KEYS[k.code] || `Key ${k.code}`}
                                        </span>
                                        <Button
                                            size="small"
                                            style={{ color: 'var(--danger)', padding: '4px 8px' }}
                                            onClick={() => setTriggerKeys(triggerKeys.filter((_, i) => i !== idx))}
                                        >
                                            Remove
                                        </Button>
                                    </div>

                                    <div style={{ display: 'flex', gap: '8px' }}>
                                        <Button
                                            variant={k.state === 1 ? 'primary' : 'default'}
                                            size="small"
                                            onClick={() => updateTriggerKey(idx, { state: k.state === 1 ? 0 : 1 })}
                                            style={{ flex: 1, fontSize: '0.75rem' }}
                                        >
                                            {k.state === 1 ? 'PRESS' : 'RELEASE'}
                                        </Button>
                                        <Button
                                            variant={k.suppress ? 'secondary' : 'default'}
                                            size="small"
                                            onClick={() => updateTriggerKey(idx, { suppress: !k.suppress })}
                                            style={{ flex: 1, fontSize: '0.75rem', color: k.suppress ? 'var(--warning)' : '' }}
                                        >
                                            WITHHOLD
                                        </Button>
                                        <Button
                                            variant={k.ignoreRepeat ? 'secondary' : 'default'}
                                            size="small"
                                            onClick={() => updateTriggerKey(idx, { ignoreRepeat: !k.ignoreRepeat })}
                                            style={{ flex: 1, fontSize: '0.75rem', color: k.ignoreRepeat ? 'var(--danger)' : '' }}
                                        >
                                            BREAK REPEAT
                                        </Button>
                                    </div>
                                </div>
                            ))}

                            {triggerKeys.length === 0 && (
                                <div style={{ padding: '20px', textAlign: 'center', color: 'var(--text-dim)', fontSize: '0.9rem', border: '1px dashed var(--glass-border)', borderRadius: '12px' }}>
                                    No trigger keys defined.
                                </div>
                            )}
                        </div>
                    </div>
                </div>

                <div className="glass" style={{ padding: '24px', display: 'flex', flexDirection: 'column' }}>
                    <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', marginBottom: '16px' }}>
                        <label style={{ fontSize: '0.85rem', fontWeight: '700', color: 'var(--text-secondary)', textTransform: 'uppercase' }}>
                            Output Sequence
                        </label>
                        <Button variant="secondary" size="small" onClick={addSequenceStep}>
                            + Add Step
                        </Button>
                    </div>

                    <div style={{ flex: 1, overflowY: 'auto', maxHeight: '500px' }}>
                        {sequence.map((step, idx) => (
                            <div key={idx} style={{
                                display: 'flex',
                                gap: '12px',
                                marginBottom: '8px',
                                alignItems: 'center',
                                padding: '8px',
                                background: 'rgba(255,255,255,0.01)',
                                borderRadius: '8px'
                            }}>
                                <span style={{ color: 'var(--text-dim)', fontSize: '0.75rem', width: '20px' }}>{idx + 1}</span>
                                <select
                                    className="glass-input"
                                    style={{ flex: 1, padding: '8px' }}
                                    value={step.code}
                                    onChange={e => updateSequenceStep(idx, 'code', Number(e.target.value))}
                                >
                                    {Object.entries(COMMON_KEYS).sort((a, b) => a[1].localeCompare(b[1])).map(([c, n]) => (
                                        <option key={c} value={c}>{n}</option>
                                    ))}
                                </select>
                                <select
                                    className="glass-input"
                                    style={{ width: '100px', padding: '8px' }}
                                    value={step.value}
                                    onChange={e => updateSequenceStep(idx, 'value', Number(e.target.value))}
                                >
                                    <option value={1}>Press</option>
                                    <option value={0}>Release</option>
                                </select>
                                <Button
                                    size="small"
                                    style={{ color: 'var(--danger)' }}
                                    onClick={() => removeSequenceStep(idx)}
                                >
                                    ×
                                </Button>
                            </div>
                        ))}

                        {sequence.length === 0 && (
                            <div style={{ padding: '20px', textAlign: 'center', color: 'var(--text-dim)', fontSize: '0.9rem', border: '1px dashed var(--glass-border)', borderRadius: '12px' }}>
                                No output sequence defined.
                            </div>
                        )}
                    </div>
                </div>
            </div>
        </div>
    );
};

export default MacroBuilder;
