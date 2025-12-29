import { useState, useEffect } from 'react';
import './App.css';
import Sidebar from './components/Sidebar/Sidebar';
import ContextMonitor from './components/ContextMonitor';
import LiveLogs from './components/LiveLogs/LiveLogs';
import Configs from './components/Configs/Configs';
import type { ViewType } from './types';

function App() {
  const [currentView, setCurrentView] = useState<ViewType>('logs');
  const [logs, setLogs] = useState<string[]>([]);
  const [macros, setMacros] = useState<any>({});
  const [filters, setFilters] = useState<string[]>([]);

  useEffect(() => {
    // Fetch macros
    fetch('http://localhost:9224/api/macros')
      .then(res => res.json())
      .then(data => setMacros(data))
      .catch(console.error);

    // Fetch filters
    fetch('http://localhost:9224/api/filters')
      .then(res => res.json())
      .then(data => setFilters(data))
      .catch(console.error);

    // WebSocket for logs
    const ws = new WebSocket('ws://localhost:9224');
    ws.onopen = () => console.log('WebSocket Connected');
    ws.onmessage = (event) => {
      setLogs(prev => {
        const newLogs = [...prev, event.data];
        if (newLogs.length > 500) return newLogs.slice(-500); // Keep last 500
        return newLogs;
      });
    };
    ws.onerror = (e) => console.error('WebSocket error:', e);

    return () => ws.close();
  }, []);

  const saveMacros = (newMacros: any) => {
    fetch('http://localhost:9224/api/macros', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(newMacros)
    })
      .then(() => {
        alert('Macros saved successfully!');
        setMacros(newMacros);
      })
      .catch(err => alert('Error saving macros: ' + err.message));
  };

  const setEventFilters = (newFilters: string[]) => {
    setFilters(newFilters);
    fetch('http://localhost:9224/api/filters', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(newFilters)
    }).catch(console.error);
  };

  const clearLogs = () => setLogs([]);

  return (
    <div className="dashboard-layout">
      <Sidebar
        currentView={currentView}
        onViewChange={(view) => setCurrentView(view as ViewType)}
      />

      <main className="main-content">
        <ContextMonitor />
        {currentView === 'logs' && (
          <LiveLogs
            logs={logs}
            filters={filters}
            onSetFilters={setEventFilters}
            onClearLogs={clearLogs}
          />
        )}

        {currentView === 'configs' && (
          <Configs
            macros={macros}
            onSave={saveMacros}
          />
        )}
      </main>
    </div>
  );
}

export default App;
