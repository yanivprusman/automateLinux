import React from 'react';

interface InputProps extends React.InputHTMLAttributes<HTMLInputElement | HTMLTextAreaElement> {
    label?: string;
    multiline?: boolean;
}

const Input: React.FC<InputProps> = ({ label, multiline, className = '', ...props }) => {
    const InputComponent = multiline ? 'textarea' : 'input';

    return (
        <div className="form-group" style={{ marginBottom: '16px', width: '100%' }}>
            {label && <label style={{
                display: 'block',
                marginBottom: '8px',
                fontSize: '0.85rem',
                color: 'var(--text-secondary)',
                fontWeight: '500'
            }}>{label}</label>}
            <InputComponent
                className={`glass-input ${className}`}
                style={{
                    width: '100%',
                    padding: '12px 16px',
                    background: 'rgba(255, 255, 255, 0.03)',
                    border: '1px solid var(--glass-border)',
                    borderRadius: '12px',
                    color: 'var(--text-primary)',
                    fontSize: '0.95rem',
                    outline: 'none',
                    transition: 'var(--transition-fast)',
                    ...(multiline ? { resize: 'vertical', minHeight: '100px' } : {})
                }}
                {...(props as any)}
            />
        </div>
    );
};

export default Input;
