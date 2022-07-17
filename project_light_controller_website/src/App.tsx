import { useContext } from 'react';
import { Link } from "react-router-dom";
import './App.css';
import { CurrentStatusContext } from './contexts/current-status-context'
import LightControls from './LightControls';

function App() {
  const currentStatus = useContext(CurrentStatusContext);
  return (
    <div className="App">
      <section className="current-status">
        <h2>Current Status</h2>
        <div className="status-property">
          <div className="status-label">Turned On</div>
          <div className="status-value">{currentStatus.turnedOn ? "Yes" : "No"}</div>
        </div>
        <div className="status-property">
          <div className="status-label">Brightness</div>
          <div className="status-value">{currentStatus.brightness} %</div>
        </div>
        <div className="status-property">
          <div className="status-label">Light Level</div>
          <div className="status-value">{currentStatus.lightLevel} %</div>
        </div>
      </section>
      <section className="light-control">
        <LightControls />
      </section>
      <section className='navigation'>
        <Link to="settings">Settings</Link>
      </section>
    </div>
  );
}

export default App;
