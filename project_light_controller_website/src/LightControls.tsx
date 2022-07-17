import { API_ENDPOINT } from './api/apiEndpoint';
import React, { useState, useContext } from 'react';
import './LightControls.css';
import axios from 'axios';
import qs from 'qs';
import { CurrentStatusContext } from './contexts/current-status-context'
import Slider from '@mui/material/Slider'

function LightControls() {
    const [error, setError] = useState();
    const [canClick, setCanClick] = useState(true);
    const currentStatus = useContext(CurrentStatusContext);
    const currentBrightness = Number.isNaN(Number(currentStatus.brightness)) ? 0 : Number(currentStatus.brightness);
    const [currentSliderValue, setCurrentSliderValue] = useState(Number(currentBrightness));
    const [brightnessChanging, setBrightnessChanging] = useState(false);

    // Update the slider value if changed in current status and not from user input
    if(currentBrightness !== currentSliderValue && !brightnessChanging) {
        setCurrentSliderValue(currentBrightness);
    }

    const lightsOff = () => {
        setCanClick(false);
        axios.get(API_ENDPOINT + '/lights-off')
            .then(result => setCanClick(true))
            .catch(error => {
                setCanClick(true);
                setError(error);
            });
    }
    
    const lightsOn = () => {
        setCanClick(false);
        axios.get(API_ENDPOINT + '/lights-on')
            .then(result => setCanClick(true))
            .catch(error => {
                setCanClick(true);
                setError(error);
            });
    }

    const setBrightness = (brightness: number) => {
        const url = API_ENDPOINT + '/brightness';
        const data = { brightness: brightness };
        const options = {
            method: 'POST',
            headers: { 'content-type': 'application/x-www-form-urlencoded' },
            data: qs.stringify(data),
            url
        };
        axios(options).then(result => setTimeout(() => setBrightnessChanging(false), 650));
    }

    const onBrightnessChanged = (event: React.SyntheticEvent | Event, value: number | Array<number>) => {
        if(!Array.isArray(value)) {
            setCurrentSliderValue(value as number);
        }
        setBrightnessChanging(true);
    }

    const onBrightnessCommitted = (event: React.SyntheticEvent | Event, value: number | Array<number>) => {
        if(!Array.isArray(value)) {
            setBrightness(value as number);
        }
    }

    const action = currentStatus.turnedOn ? "Turn Off" : "Turn On";
    const actionButtonClass = canClick ? 'light-control-button' : 'light-control-button light-control-button-disabled';
    const actionButtonhandler = currentStatus.turnedOn ? lightsOff : lightsOn;

    return (
        <div className='light-controls'>
            <div className='brightness-controls'>
                <div className='brightness-label'>Brightness</div>
                <Slider aria-label="Brightness" disabled={!currentStatus.turnedOn} onChange={onBrightnessChanged} onChangeCommitted={onBrightnessCommitted} min={0} max={100} value={currentSliderValue} />
            </div>
            <div className='onoff-controls'>
                <button className={actionButtonClass} onClick={actionButtonhandler} disabled={!canClick}>{action}</button>
                <span className='light-control-error'>{error == null ? "" : "An error occurred."}</span>
            </div>
        </div>
    );
}

export default LightControls;