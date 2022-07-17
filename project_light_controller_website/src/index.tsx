import React from 'react';
import { createRoot } from 'react-dom/client';
import { BrowserRouter, Route, Routes } from "react-router-dom";
import './index.css';
import App from './App';
import AppSettings from './AppSettings';
import { CurrentStatusContextProvider } from './contexts/current-status-context'

const container = document.getElementById('root') as HTMLInputElement;
const root = createRoot(container);
root.render(<React.StrictMode>
  <CurrentStatusContextProvider>
    <BrowserRouter>
      <Routes>
        <Route path="/" element={<App/>} />
        <Route path="settings/*" element={<AppSettings/>} />
      </Routes>
    </BrowserRouter>
  </CurrentStatusContextProvider>
</React.StrictMode>);