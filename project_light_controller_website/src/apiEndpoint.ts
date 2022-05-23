let backendHost;

const hostname = window && window.location && window.location.hostname;

if(hostname === 'localhost') {
  backendHost = 'http://192.168.1.50';
} else {
    backendHost = '';
}

export const API_ENDPOINT = backendHost;