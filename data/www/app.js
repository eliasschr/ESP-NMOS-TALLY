(() => {
  const message = document.querySelector('#message');
  const status = document.querySelector('#status');
  const brightness = document.querySelector('#brightness');
  const value = document.querySelector('#brightness-value');
  const showMessage = text => { message.textContent = text; };
  const request = async (path, options = {}) => {
    const secret = document.querySelector('#provisioning-secret').value;
    options.headers = { ...(options.headers || {}), 'X-Provisioning-Secret': secret };
    const response = await fetch(path, options);
    const body = await response.json();
    if (!response.ok) throw new Error(body.error || 'Request failed');
    return body;
  };
  const refresh = async () => {
    const data = await (await fetch('/api/status')).json();
    document.querySelector('#mode').textContent = `Mode: ${data.wifiMode}`;
    status.replaceChildren(...Object.entries(data).filter(([key]) => key !== 'nmos').map(([key, text]) => {
      const term = document.createElement('dt'); term.textContent = key;
      const detail = document.createElement('dd'); detail.textContent = text;
      return [term, detail];
    }).flat());
    if (data.wifiMode !== 'provisioning') return;
    document.querySelector('#network-section').hidden = false;
    document.querySelector('#test-section').hidden = false;
    document.querySelector('[name=name]').value = data.deviceName;
  };
  document.querySelector('#network-form').addEventListener('submit', async event => {
    event.preventDefault();
    try { await request('/api/network', { method: 'POST', body: new FormData(event.target) }); showMessage('Saved. Connecting to the selected Wi-Fi network…'); }
    catch (error) { showMessage(error.message); }
  });
  document.querySelectorAll('[data-state]').forEach(button => button.addEventListener('click', async () => {
    try { await request('/api/tally', { method: 'POST', body: new URLSearchParams({ state: button.dataset.state, brightness: brightness.value }) }); showMessage(`Set ${button.dataset.state}.`); refresh(); }
    catch (error) { showMessage(error.message); }
  }));
  brightness.addEventListener('input', () => { value.textContent = brightness.value; });
  refresh().catch(error => showMessage(error.message));
})();
