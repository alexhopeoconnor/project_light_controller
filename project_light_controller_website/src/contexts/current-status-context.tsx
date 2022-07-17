import axios from 'axios';
import { API_ENDPOINT } from '../api/apiEndpoint';
import React, { createContext, useState, useEffect, ReactNode } from 'react';

// Declare initial status
var initialStatus:CurrentStatus = {
    turnedOn: false,
    brightness: "Unknown",
    lightLevel: "Unknown"
};

// Create current status context
const CurrentStatusContext = createContext(initialStatus);

// Create the current status context provider
type CurrentStatusContextProviderProps = { children: ReactNode }
function CurrentStatusContextProvider({ children }:CurrentStatusContextProviderProps) {
    // Define backing state for the provider
    const [currentStatus, setCurrentStatus] = useState<CurrentStatus>(initialStatus);

    // Define method to retrieve updated state from the endpoint
    const updateCurrentStatus = () => {
        axios.get<CurrentStatus>(API_ENDPOINT + '/current-status')
          .then(result => setCurrentStatus(result.data));
    }

    // Define interval to automatically update state internally only
    useEffect(() => {
        const currentStatusTimer = setInterval(() => updateCurrentStatus(), 1000);
        updateCurrentStatus();

        return () => {
            clearInterval(currentStatusTimer);
        };
    }, []);

    // Return the provider component
    return (
      <CurrentStatusContext.Provider value={currentStatus}>
        {children}
      </CurrentStatusContext.Provider>
    );
  }

export { CurrentStatusContext, CurrentStatusContextProvider };