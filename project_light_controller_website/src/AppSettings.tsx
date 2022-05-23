import React from 'react';
import { Link } from "react-router-dom";
import './AppSettings.css';

function AppSettings() {
  return (
    <div className="AppSettings">
      <section className='navigation'>
        <Link to="/">Light Controls</Link>
      </section>
    </div>
  );
}

export default AppSettings;
