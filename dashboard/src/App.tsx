import React, { useState, useEffect, useRef } from 'react';
import './App.css';

interface Macro {
  message: string;
  trigger: {
    keys: [number, number, boolean][];
  };
  sequence: [number, number][];
  hasHandler: boolean;
}

const COMMON_KEYS: { [key: number]: string } = {
  28: 'ENTER',
  96: 'KP_ENTER',
  29: 'LEFT_CTRL',
  42: 'LEFT_SHIFT',
  56: 'LEFT_ALT',
  1: 'ESC',
  47: 'V',
  105: 'LEFT',
  106: 'RIGHT',
  103: 'UP',
  108: 'DOWN',
  272: 'BTN_LEFT',
  273: 'BTN_RIGHT',
  277: 'BTN_FORWARD',
  278: 'BTN_BACK',
};

function App() {
  const [logs, setLogs] = useState<string[]>([]);
  const [macros, setMacros] = useState<any>({});
  const [filters, setFilters] = useState<number[]>([]);
  const [editingMacros, setEditingMacros] = useState('');
  const logEndRef = useRef<HTMLDivElement>(null);

  useEffect(() => {
    // Fetch macros
    fetch('http://localhost:9224/api/macros')
      .then(res => res.json())
      .then(data => {
        setMacros(data);
        setEditingMacros(JSON.stringify(data, null, 2));
      });

    // WebSocket for logs
    const ws = new WebSocket('ws://localhost:9224');
    ws.onmessage = (event) => {
      setLogs(prev => [...prev.slice(-200), event.data]);
    };

    return () => ws.close();
  }, []);

  useEffect(() => {
    logEndRef.current?.scrollIntoView({ behavior: 'smooth' });
  }, [logs]);

  const saveMacros = () => {
    try {
      const parsed = JSON.parse(editingMacros);
      fetch('http://localhost:9224/api/macros', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(parsed)
      }).then(() => alert('Macros saved!'));
    } catch (e) {
      alert('Invalid JSON');
    }
  };

  const toggleFilter = (code: number) => {
    const newFilters = filters.includes(code)
      ? filters.filter(c => c !== code)
      : [...filters, code];
    setFilters(newFilters);
    fetch('http://localhost:9224/api/filters', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(newFilters)
    });
  };

  return (
    <div className="dashboard">
      <header className="glass">
        <h1>AutomateLinux Live Dashboard</h1>
        <div className="status">
          <span className="dot pulse"></span> Daemon Active
        </div>
      </header>

      <main>
        <div className="left-panel">
          <section className="glass logs-section">
            <h2>Live Event Stream</h2>
            <div className="log-container">
              {logs.map((log, i) => (
                <div key={i} className="log-entry">
                  {log}
                </div>
              ))}
              <div ref={logEndRef} />
            </div>
          </section>

          <section className="glass filters-section">
            <h2>Granular Logging Filters</h2>
            <div className="filter-grid">
              {Object.entries(COMMON_KEYS).map(([code, name]) => (
                <label key={code} className={`filter-chip ${filters.includes(Number(code)) ? 'active' : ''}`}>
                  <input
                    type="checkbox"
                    checked={filters.includes(Number(code))}
                    onChange={() => toggleFilter(Number(code))}
                  />
                  {name} ({code})
                </label>
              ))}
            </div>
          </section>
        </div>

        <div className="right-panel">
          <section className="glass macros-section">
            <h2>Macro / Binding Editor</h2>
            <textarea
              className="macro-editor"
              value={editingMacros}
              onChange={(e) => setEditingMacros(e.target.value)}
              spellCheck={false}
            />
            <button className="btn-primary" onClick={saveMacros}>Apply Bindings</button>
          </section>
        </div>
      </main>
    </div>
  );
}

export default App;
