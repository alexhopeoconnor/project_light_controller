let backendHost;

const hostname = window && window.location && window.location.hostname;

if(hostname === 'localhost') {
  backendHost = 'http://192.168.15.50';
} else {
  backendHost = '';
}

export const API_ENDPOINT = backendHost;