(() => {
  const requestTimeoutMs = 15000;
  const networkLimits = { name: 32, ssid: 32, password: 64 };
  const messages = {
    authorization: 'Authorization failed. Check the provisioning secret shown in the USB serial console.',
    cooldown: 'Too many failed authorization attempts. Please wait before trying again.',
    invalid: 'Invalid network configuration. Check the device name, Wi-Fi name and password.',
    tooLarge: 'The configuration request is too large. Check your input or contact the firmware maintainer.',
    storage: 'The ESP8266 could not save its configuration. Check device storage.',
    expired: 'Provisioning is no longer active. Start a new session using PROVISION over USB.',
    network: 'Connection to ESP-NMOS-TALLY was interrupted. The device may be switching Wi-Fi networks.',
    timeout: 'The device did not respond in time. Verify your connection and try again.',
    unverified: 'Wi-Fi connection has not yet been verified. Reconnect to your normal Wi-Fi and open the device status page.'
  };
  const connectionProgressFor = wifiState => ({
    provisioning: 'Ready to configure',
    unconfigured: 'Wi-Fi has not been configured',
    connecting: 'Waiting for device connection',
    connected: 'Connection confirmed',
    reconnecting: 'Connection could not be verified',
    cooling_down: 'Connection retry is cooling down',
    failed: 'Connection attempt failed'
  })[wifiState] || 'Connection could not be verified';

  const messageForHttpFailure = (status, code) => {
    if (code === 'provisioning_inactive') return messages.expired;
    if (code === 'missing_device_name') return 'Enter a device name.';
    if (code === 'invalid_device_name') return 'Use a device name no longer than 32 bytes.';
    if (code === 'missing_wifi_ssid') return 'Enter a Wi-Fi network name.';
    if (code === 'wifi_ssid_too_long') return 'Use a Wi-Fi network name no longer than 32 bytes.';
    if (code === 'invalid_wifi_password') return 'Use a Wi-Fi password no longer than 64 bytes.';
    if (code === 'malformed_form') return 'The configuration form could not be read. Reload the page and try again.';
    if (status === 403) return messages.authorization;
    if (status === 429) return messages.cooldown;
    if (status === 400) return messages.invalid;
    if (status === 413) return messages.tooLarge;
    if (status === 500) return messages.storage;
    return 'The device could not complete this request. Try again after checking the device status.';
  };
  const byteLength = value => {
    if (typeof TextEncoder !== 'undefined') return new TextEncoder().encode(value).length;
    return encodeURIComponent(value).replace(/%[0-9A-F]{2}|./gi, 'x').length;
  };
  const validateNetworkValues = values => {
    for (const [field, maximum] of Object.entries(networkLimits)) {
      if (byteLength(values.get(field) || '') > maximum) return field;
    }
    return null;
  };
  const encodeNetworkForm = values => {
    const encoded = new URLSearchParams();
    // Copy values captured before controls are disabled. This avoids relying on
    // FormData iteration support and keeps Safari's request body explicit.
    for (const field of ['name', 'ssid', 'password']) encoded.set(field, values.get(field) || '');
    return encoded;
  };
  const apiRequest = async (path, options = {}) => {
    const controller = new AbortController();
    const timeout = setTimeout(() => controller.abort(), requestTimeoutMs);
    try {
      const response = await fetch(path, { ...options, signal: controller.signal });
      let responseBody = {};
      try { responseBody = await response.json(); } catch (_) { /* Status code remains authoritative. */ }
      if (!response.ok) throw { type: 'http', status: response.status, code: responseBody.code };
      return responseBody;
    } catch (error) {
      if (error.type === 'http') throw error;
      if (error.name === 'AbortError') throw { type: 'timeout' };
      throw { type: 'network' };
    } finally { clearTimeout(timeout); }
  };
  const errorMessage = error => error.type === 'http' ? messageForHttpFailure(error.status, error.code) :
    error.type === 'timeout' ? messages.timeout : messages.network;

  if (typeof module !== 'undefined') module.exports = { byteLength, connectionProgressFor, encodeNetworkForm, errorMessage, messageForHttpFailure, validateNetworkValues };
  if (typeof document === 'undefined') return;

  const form = document.querySelector('#network-form');
  const status = document.querySelector('#status');
  const feedback = document.querySelector('#feedback');
  const message = document.querySelector('#message');
  const alert = document.querySelector('#alert');
  const progress = document.querySelector('#connection-progress');
  const fieldError = document.querySelector('#network-fields-error');
  const brightness = document.querySelector('#brightness');
  const value = document.querySelector('#brightness-value');
  const showFeedback = (kind, text) => {
    feedback.hidden = false;
    feedback.dataset.kind = kind;
    message.textContent = kind === 'error' ? '' : text;
    alert.textContent = kind === 'error' ? text : '';
  };
  const showProgress = text => { progress.textContent = text; };
  const setNetworkFormPending = pending => form.querySelectorAll('input, button').forEach(element => { element.disabled = pending; });
  const request = (path, options = {}) => apiRequest(path, { ...options, headers: { ...(options.headers || {}), 'X-Provisioning-Secret': document.querySelector('#provisioning-secret').value } });
  const updateConnectionProgress = data => {
    if (data.wifiState === 'connected') {
      showProgress(connectionProgressFor(data.wifiState));
      showFeedback('success', 'Connection confirmed by ESP-NMOS-TALLY.');
    } else if (data.wifiState) showProgress(connectionProgressFor(data.wifiState));
  };
  const refresh = async () => {
    const response = await fetch('/api/status');
    if (!response.ok) throw { type: 'http', status: response.status };
    const data = await response.json();
    document.querySelector('#mode').textContent = `Mode: ${data.wifiState || data.wifiMode}`;
    status.replaceChildren(...Object.entries(data).filter(([key]) => key !== 'nmos').map(([key, text]) => {
      const term = document.createElement('dt'); term.textContent = key;
      const detail = document.createElement('dd'); detail.textContent = text;
      return [term, detail];
    }).flat());
    updateConnectionProgress(data);
    if (data.wifiMode !== 'provisioning') return;
    document.querySelector('#network-section').hidden = false;
    document.querySelector('#test-section').hidden = false;
    document.querySelector('[name=name]').value = data.deviceName;
    if (feedback.hidden) showFeedback('status', 'Ready to configure.');
  };

  form.addEventListener('submit', async event => {
    event.preventDefault();
    const values = new FormData(form);
    showProgress('Validating input');
    fieldError.textContent = '';
    const invalidField = validateNetworkValues(values);
    if (invalidField) {
      const label = invalidField === 'ssid' ? 'Wi-Fi network' : invalidField === 'name' ? 'Device name' : 'Wi-Fi password';
      fieldError.textContent = `${label} must be no more than ${networkLimits[invalidField]} bytes.`;
      showFeedback('error', messages.invalid);
      return;
    }
    const requestBody = encodeNetworkForm(values);
    setNetworkFormPending(true);
    showProgress('Saving configuration');
    showFeedback('status', 'Saving configuration…');
    try {
      // This avoids multipart boundary overhead and is parsed by ESP8266WebServer.
      await request('/api/network', { method: 'POST', body: requestBody });
      showProgress('Configuration saved');
      showFeedback('success', 'Configuration saved. The device is switching Wi-Fi networks.');
      setTimeout(() => {
        showProgress('Switching networks');
        showFeedback('warning', `The setup network may now disconnect. ${messages.unverified}`);
      }, 0);
      setTimeout(() => showProgress('Waiting for device connection'), 750);
      setTimeout(() => showProgress('Connection could not be verified'), 5000);
    } catch (error) {
      setNetworkFormPending(false);
      showProgress('Ready to configure');
      showFeedback('error', errorMessage(error));
    }
  });
  document.querySelectorAll('[data-state]').forEach(button => button.addEventListener('click', async () => {
    try {
      await request('/api/tally', { method: 'POST', body: new URLSearchParams({ state: button.dataset.state, brightness: brightness.value }) });
      showFeedback('success', `Set ${button.dataset.state}.`);
      refresh();
    } catch (error) { showFeedback('error', errorMessage(error)); }
  }));
  brightness.addEventListener('input', () => { value.textContent = brightness.value; });
  refresh().catch(error => showFeedback('error', errorMessage(error)));
})();
